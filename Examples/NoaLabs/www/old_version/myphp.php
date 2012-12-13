<?php
	$file = "readfile.txt";
	$fp = fopen($file, 'r') or exit("Unable to open file!");
	$i = 0;
	$arr = array();

	while (!feof($fp)) {
		$string = fgets($fp);
		$newstring = explode("	", $string);		
		array_push($arr, $newstring[1]);
	}
	
//      fputs($fp, "4321");
	$json_string = json_encode($arr);
	echo "getfile($json_string)";
	fclose($fp);
?>

