<?php
$syncCredentialsFile = '/opt/EveryCook/sync/login_cred';
$syncCredentialsDefault = 'user=demo&pw=demo';
$wlanConfigFile = '/etc/wpa_supplicant/wpa_supplicant.conf';

session_start();

if (!isset($_SESSION['ignoreWLAN']) || !$_SESSION['ignoreWLAN']){
	$wlanConfigSize = filesize($wlanConfigFile);
	if ($wlanConfigSize < 100){ //"empty" file is about 72 bytes
		header("Location: /install.php");
		exit;
	}
	$data = array();
	$returncode = -1;
	exec('/sbin/ifconfig wlan0 | grep "inet addr"', $data, $returncode);
	$wlan_connected = $returncode === 0;
	if (!$wlan_connected){
		header("Location: /install.php");
		exit;
	}
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

if (!$userSet){
	header("Location: /install.php");
	exit;
}

$ipArray = array();
$returncode = -1;
exec('/sbin/ip -f inet addr show dev wlan0 | sed -n \'s/^ *inet *\([.0-9]*\).*/\1/p\'', $ipArray, $returncode);
if (count($ipArray)>0){
	$wlan_ip = $ipArray[0];
	
	if (strlen($wlan_ip) > 0 && $wlan_ip != $_SERVER["SERVER_ADDR"]){
		header("Location: http://".$wlan_ip."/db/");
		exit;
	}
}

//if all is OK, redirect do Recipedatabase
header("Location: /db/");
?>