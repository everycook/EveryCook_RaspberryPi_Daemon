#!/bin/bash

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# with ". script.sh" the script will run in current context not as a new process
. installVars.sh

if [ "$?" != "0" ]; then
	echo "could not load settings from installVars.sh" 1>&2
	exit 1
fi
if [ -z "$installDir" ]; then
	echo "could not load settings from installVars.sh" 1>&2
	exit 1
fi
#@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

echo "create dirs"

mkdir $installDir
mkdir $installDir/sync
mkdir $installDir/sync/recipes
chown $userToUse:$userToUse $installDir/
chown $userToUse:$webuser $installDir/sync
chown $userToUse:$userToUse $installDir/sync/recipes
chmod 0775 $installDir/
chmod 0775 $installDir/sync
chmod 0775 $installDir/sync/recipes


mkdir $webdir
chown $userToUse:$webuser $webdir
chmod 0775 $webdir
ln -s $webdir $installDir/web

pushd . > /dev/null
cd $installDir/sync

echo "create files"

cat << EOF > init_db.sql
CREATE DATABASE IF NOT EXISTS ec;
CREATE DATABASE IF NOT EXISTS ec_priv;
GRANT ALL PRIVILEGES ON ec.* TO 'ec'@'localhost';
GRANT ALL PRIVILEGES ON ec_priv.* TO 'ec'@'localhost';
GRANT SUPER ON *.* TO 'ec'@'localhost';

EOF

cat << EOF > init_db.sh
#!/bin/bash
cat init_db.sql | mysql -h localhost -uroot -p$mysqlRootLogin
EOF
chmod 0775 init_db.sh
./init_db.sh

cat << EOF > login_cred
user=demo&pw=demo
EOF
chmod 0620 login_cred
chown $userToUse:$webuser login_cred
chmod 0660 login_cred

cat << EOF > sync_privdata.sh
#!/bin/bash
#rm $installDir/sync/privdata.sql
truncate -s 0 $installDir/sync/privdata.sql
wlanIP=`/sbin/ip -f inet addr show dev wlan0 | sed -n 's/^ *inet *\([.0-9]*\).*/\1/p'`
wget --post-file=$installDir/sync/login_cred -c http://www.everycook.org/db/sync/exportPrivData?ip=\$wlanIP -O $installDir/sync/privdata.sql -o $installDir/sync/sync_privdata.log

if [ "\$?" != "0" ]; then
	echo "Error getting private data.";
else
	if [ -s $installDir/sync/privdata.sql ];
	then
	#	chmod 0664 $installDir/sync/privdata.sql
	##	chown $userToUse:$webuser $installDir/sync/privdata.sql
	        firstLine="\`head -1 $installDir/sync/privdata.sql | cut -c 1-7\`";
	        if [ "\$firstLine" == "#Error:" ];
	        then
	                head -1 $installDir/sync/privdata.sql | cut -c 2-
	        else
	                echo "import priv data";
	                cat $installDir/sync/privdata.sql | mysql -h localhost -uec
	        fi
	else
	        echo "Error getting private data.";
	fi
fi
EOF
chmod 0775 sync_privdata.sh
touch sync_privdata.log
chown $userToUse:$webuser sync_privdata.log
chmod 0664 sync_privdata.log
touch privdata.sql
chown $userToUse:$webuser privdata.sql
chmod 0664 privdata.sql


cat << EOF > sync_php.sh
#!/bin/bash
rsync -az --log-file=$installDir/sync/sync_php.log --delete --ignore-errors --exclude-from=$installDir/sync/notDelete --include=protected/extensions/fancybox/assets rsync://everycook.org/php $webdir
if [ "\$?" != "0" ]; then
	echo "Error getting php files.";
else
	mkdir $webdir/assets
	mkdir $webdir/cache
	mkdir $webdir/protected/runtime
	mkdir $webdir/protected/config
	cp $installDir/sync/main_yii_config.php $webdir/protected/config/main.php
	chmod 0755 $webdir/protected/config/main.php
	chmod 0775 $webdir/assets
	chmod 0775 $webdir/cache
	chmod 0775 $webdir/protected/extensions/fancybox/assets
	chmod 0775 $webdir/protected/runtime
	sudo chown -R $userToUse:$webuser $webdir/
	sudo chown -R $webuser:$webuser $webdir/cache/
	sudo chown -R $webuser:$webuser $webdir/img/
	echo "files synced";
fi
EOF
chmod 0775 sync_php.sh


cat << EOF > notDelete
/protected/runtime
/assets
/cache
/protected/config
EOF
chmod 0776 notDelete


cat << EOF > sync_recipes.sh
#!/bin/bash
rsync -az --log-file=$installDir/sync/sync_recipes.log rsync://everycook.org/recipes $installDir/sync/recipes
if [ "\$?" != "0" ]; then
	echo "Error getting recipe database redologs.";
else
	echo "files synced";

	mv $installDir/sync/lastRecipeSyncDate.txt $installDir/sync/recipeSyncDate.txt
	echo \`date +"%F %R:%S"\` > $installDir/sync/lastRecipeSyncDate.txt
	#TODO check if there is a newer EC_INITIAL_ file!
	
	if [ -r $installDir/sync/recipeSyncDate.txt ];
	then
	        echo "import changes";
	        mysqlbinlog --short-form --start-datetime="\`cat $installDir/sync/recipeSyncDate.txt\`" $installDir/sync/recipes/mysql-bin.[0-9]* | mysql -h localhost -uec
	else
	        echo "import full mysqldump";
		gzip -d -c $installDir/sync/recipes/EC_INITIAL_\`cat $installDir/sync/recipes/EC_INITIAL_DATE.txt\`.sql.gz | mysql -h localhost -uec --database=ec
	        echo "import changes since mysqldump";
	        mysqlbinlog --short-form --start-datetime="\`cat $installDir/sync/recipes/EC_INITIAL_DATE.txt\` 00:00:00" $installDir/sync/recipes/mysql-bin.[0-9]* | mysql -h localhost -uec
	fi
fi
EOF
chmod 0775 sync_recipes.sh

chown $userToUse:$userToUse $installDir/sync/*
chown $userToUse:$webuser $installDir/sync/login_cred
popd > /dev/null

