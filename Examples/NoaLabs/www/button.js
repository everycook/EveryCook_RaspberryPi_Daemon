function PressDown(buttonID,fun) {
	if (document.getElementById(buttonID).value == "1")
		document.getElementById(buttonID).value = "0";
	else
		document.getElementById(buttonID).value = "1";
	fun();
}
function PressOnce(buttonID,fun) {
	document.getElementById(buttonID).value = "0";
	fun();
}
