<?php
if (isset($_REQUEST['w'])) $w=$_REQUEST['w']; else $w=1;
if (isset($_REQUEST['h'])) $h=$_REQUEST['h']; else $h=1;
if (isset($_REQUEST['b'])) $c=$_REQUEST['b']; else $b="000000";
if (isset($_REQUEST['c'])) $c=$_REQUEST['c']; else $c="ffffff";

// example1.php
 
// set the HTTP header type to PNG
header("Content-type: image/png"); 
 

 
// create a pointer to a new true colour image
$im = ImageCreateTrueColor($w, $h); 
 
// sets background to red
//$red = ImageColorAllocate($im, 255, 0, 0);
$bg=intval($b,16);
ImageFillToBorder($im, 0, 0, $bg, $bg);
$fg=intval($c,16);
$borderwidth=2;
imagefilledrectangle($im,$borderwidth,$borderwidth,$w-$borderwidth,$h-$borderwidth,$fg);
// send the new PNG image to the browser
ImagePNG($im); 
 
// destroy the reference pointer to the image in memory to free up resources
ImageDestroy($im); 
 
?> 
