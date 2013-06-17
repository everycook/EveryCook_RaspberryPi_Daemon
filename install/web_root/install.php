<?php
$syncCredentialsFile = '/opt/EveryCook/sync/login_cred';
$syncCredentialsDefault = 'user=demo&pw=demo';
$validateUrl = 'http://www.everycook.org/db/synch/validateLogin';

$wlanConfigAddScript = '/opt/EveryCook/installSettings add_wlan';
$wlanConfigChangeModeScript = '/opt/EveryCook/installSettings change_wlanmode';
$wlanReconnectScript = '/opt/EveryCook/installSettings reconnect_wlan';
$restartDeamonScript = '/opt/EveryCook/installSettings restartDeamonAndMiddleware';

$wlanConfiguredWlansScript = '/opt/EveryCook/installSettings get_configured_wlans';
$wlanModeCommand_iwconfig = '/sbin/iwconfig wlan0 | sed -n \'/Mode:/s/.*Mode:\([^ ]*\).*/\1/p\'';
$wlanModeCommand = 'ls -l /etc/network/interfaces | sed -n \'/->/s/.* -> interfaces\(.*\)/\1/p\'';
$checkWlanConnectedCommand = '/sbin/ip -f inet addr show dev wlan0 | sed -n \'s/^ *inet *\([.0-9]*\).*/\1/p\'';
$availableWlanCommand = '/sbin/iwlist wlan0 scan | sed -n \'/ESSID:/s/ *ESSID:"\([^"]*\)".*/\1/p\'';
//$checkDeamonStartedCommand = 'ps -fp `cat /opt/EveryCook/deamon/ecdeamon.pid`'
//$checkMiddlewareStartedCommand = 'ps -fp `cat /opt/EveryCook/middleware/ecmiddleware.pid`'

session_start();

require('installFunctions.php');

$addNewWLAN = false;
if(isset($_POST['addNewWLAN'])){
	$addNewWLAN = $_POST['addNewWLAN'] != '';
} else if(isset($_GET['addNewWLAN'])){
	$addNewWLAN = $_GET['addNewWLAN'] != '';
} else if(isset($_GET['wlan_ssid'])){
	$addNewWLAN = $_GET['wlan_ssid'] != '';
}

$defineUser = false;
if(isset($_POST['defineUser'])){
	$defineUser = $_POST['defineUser'] != '';
} else if(isset($_GET['defineUser'])){
	$defineUser = $_GET['defineUser'] != '';
}
	
$userSet = false;
try {
	$fp = fopen($syncCredentialsFile, 'r');
	$content = fread($fp, 200);
	if (substr($content, 0, 17) != 'user=demo&pw=demo'){
		$userSet = true;
	}
} catch (Exception $e){
}

if(isset($_GET['ignoreWLAN']) && $_GET['ignoreWLAN']){
	$_SESSION['ignoreWLAN'] = true;
	if ($userSet){
		header("Location: /db/");
		exit;
	}
}

if(isset($_GET['wlanAccessPoint']) && $_GET['wlanAccessPoint']){
	if ($_GET['wlanAccessPoint'] == "ap"){
		$result = changeWLANModeAndRestart("ap");
	} else {
		$result = changeWLANModeAndRestart("wpa");
	}
	echo "changeWLANModeAndRestart result: " . $result;
	$_SESSION['wlanAccessPoint'] = true;
	if ($userSet){
		header("Location: /db/");
		exit;
	}
}


if(isset($_GET['reconnectWLAN']) && $_GET['reconnectWLAN']){
	$result = reconnectWLAN();
}

$wlanIP='';
$wlanMode = getWlanMode();
$wlan_connected = checkWlanConnected();
showWlanInfos();


if ((!isset($_SESSION['ignoreWLAN']) || !$_SESSION['ignoreWLAN']) && ($addNewWLAN || !$wlan_connected && (!isset($_SESSION['wlanAdded']) || !$_SESSION['wlanAdded']))){
	if (addWLAN()){
		?>
		<h2>Add WLAN config</h2>
		<form method="post" action="install.php" id="login-form">
			<p class="note">Fields with <span class="required">*</span> are required.</p>

			<div class="row">
				<label class="required" for="name">wlan name<span class="required">*</span></label>
				<input type="text" id="name" name="name" value="<?php if(isset($_GET['wlan_ssid'])){ echo $_GET['wlan_ssid']; } ?>">
			</div>

			<div class="row">
				<label class="required" for="passphrase">Password <span class="required">*</span></label>
				<input type="password" id="passphrase" name="passphrase">
			</div>

			<div class="buttons">
				<input type="submit" value="add wlan" name="addNewWLAN">
			</div>
		</form>
	<?php
	}
	if (!$wlan_connected && $wlanMode != "ap"){
		echo '<a href="?ignoreWLAN=true">ignore no Wlan</a><br>';
	}
} else {
	echo '<a href="?addNewWLAN=true">add WLAN</a><br>';
}
echo '<br><br>' ."\r\n";

if ($defineUser ||!$userSet){
	if (setUserToSynch(false)){
	//if (setUserToSynch($wlan_connected)){
	?>
		<h2>Add user for synch priv data</h2>
		<form method="post" action="install.php" id="login-form">
			<p class="note">Fields with <span class="required">*</span> are required.</p>

			<div class="row">
				<label class="required" for="user">Nick name <span class="required">*</span></label>
				<input type="text" id="user" name="user">
			</div>

			<div class="row">
				<label class="required" for="pw">Password <span class="required">*</span></label>
				<input type="password" id="pw" name="pw">
			</div>

			<div class="buttons">
				<input type="submit" value="set user" name="defineUser">
			</div>
		</form>
	<?php
	}
} else {
	echo '<a href="?defineUser=true">change user for priv data synch</a><br>';
}

echo '<br><br>' ."\r\n";

if(isset($_GET['restartDeamon']) && $_GET['restartDeamon'] != ''){
	exec($restartDeamonScript);
} else {
	echo '<a href="?restartDeamon=true">restart deamon and middleware</a><br>';
}

	
?>