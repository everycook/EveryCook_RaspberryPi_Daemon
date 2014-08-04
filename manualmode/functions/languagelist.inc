<?php
function languagelist()
{
$fname="functions/ISO_639.txt";
$fp=fopen("$fname","r");
if ($fp)
{
while (!feof($fp)) 
{
$buffer = fgets($fp, 4000);
$descriptions=explode(" ",$buffer);
$descriptions[0]=strtoupper($descriptions[0]);
echo '<option value="'.$descriptions[0].'">'.$descriptions[1].' ('.$descriptions[0].')</option>';
}
}
fclose($fp);
	
}
?>
