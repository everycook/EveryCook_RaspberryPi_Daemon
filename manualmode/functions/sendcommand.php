<?php
error_reporting(E_ERROR | E_WARNING | E_PARSE | E_NOTICE);

if (isset($_REQUEST['command'])) {$command=$_REQUEST['command'];}else{$command="";}

//echo "hello world<br>";

$fw =fopen("/dev/shm/command", "w+");
if (fwrite($fw, "$command")) 
{
//echo "Writing #$command OK<br>";

}
fclose($fw);

?>
