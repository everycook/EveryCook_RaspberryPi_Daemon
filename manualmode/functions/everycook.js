function ShowData(){
	$.getJSON('status', function(data) {
	//alert(data.T0);
	var ActualMode="";
		if (data.SMODE==0) ActualMode="paused";
		else if (data.SMODE==1) ActualMode="cutting";
		else if (data.SMODE==2) ActualMode="scaling";
		else if (data.SMODE==10) ActualMode="heating up";
		else if (data.SMODE==11) ActualMode="cooking";
		else if (data.SMODE==12) ActualMode="cooling down";
		else if (data.SMODE==20) ActualMode="pressurizing";
		else if (data.SMODE==21) ActualMode="pressure cooking";
		else if (data.SMODE==22) ActualMode="pressure down";
		else if (data.SMODE==22) ActualMode="pressure off";
		else if (data.SMODE==30) ActualMode="hot";
		else if (data.SMODE==31) ActualMode="pressurised";
		else if (data.SMODE==32) ActualMode="cooled down";
		else if (data.SMODE==33) ActualMode="pressureless";
		else if (data.SMODE==34) ActualMode="weight reached";
		else if (data.SMODE==35) ActualMode="time over";
		else if (data.SMODE==39) ActualMode="recipe end";
		document.getElementById("ActBotTemp").innerHTML=data.T0;
		document.getElementById("ActPress").innerHTML=data.P0;
		document.getElementById("ActRpm").innerHTML=data.M0RPM;
		document.getElementById("ActRun").innerHTML=data.M0ON;
		document.getElementById("ActPause").innerHTML=data.M0OFF;
		document.getElementById("eta").innerHTML=data.STIME;
		document.getElementById("ActMode").innerHTML=ActualMode;
		document.getElementById("ActGrams").innerHTML=data.W0;
		document.getElementById("ActStepID").innerHTML=data.SID;
		
		if (data.SMODE==2 && intervalTimeout != 250) {
			window.clearInterval(update);
			intervalTimeout = 250;
			update = window.setInterval("ShowData()",intervalTimeout);
		} else if (intervalTimeout == 250){
			window.clearInterval(update);
			intervalTimeout = 2000;
			update = window.setInterval("ShowData()",intervalTimeout);
		}
	});
	//Actualgrams=Math.max(Actualgrams,LastGrams);
	//LastGrams=Actualgrams;
	//document.getElementById("debug").innerHTML="actual: "+Actualgrams+" set: "+SetGrams;
	ProgressBar('WeightOuter','WeightInner',Actualgrams,SetGrams,'120');
}

function MakeGraph(){
	g = new Dygraph(
		document.getElementById("demodiv"), "data.txt", {
			labels: ["Time","Temp","Press","Rpm","Weight","SetTemp","SetPress","SetRpm","SetWeight","SetMode","Mode","heaterHasPower","isHeating","noPan","lidClosed","lidLocked","pusherLocked"],
			//title: 'The Awesome EveryCook Trend Visualisation',
			legend: 'always',
			labelsDivWidth: 800,
			labelsDivStyles: { 'textAlign': 'right' },
			labelsSeparateLines: false
		}
	);
}
function DrawTemp(){
	var myChart = document.getElementById("myChart");
	var myChart_context = myChart.getContext("2d");
	myChart_context.clearRect ( 0 , 0 , myChart.width , myChart.height );
	var TextSize =10;
	var TimeAxis= new Array();
	var TempData= new Array();
	var PressData= new Array();
	myChart_context.font=TextSize+"px Arial";
	$.get("data.txt", function( text ) {
	$( ".result" ).html( text );
	var lines = text.split(/\n/);
	var xDiv = Math.round(myChart.width/lines.length);
	var xLabelNumber=20;
	var xLabelPos=0;
	var xLabelHeight=(myChart.height-xLabelPos);
	var yLabelPos=5;
	var MaxTemp=0;
	var MaxPress=0;
	//alert( xLabelHeight );
	//parse file and search max
	for (i=1;i<lines.length-1;i++)
	{
		line=lines[i].split(',');
		TimeHMS=line[0].split(' ')[1];
		TimeAxis[i] = TimeHMS.split(':')[0] + ":" + TimeHMS.split(':')[1];
		TempData[i] = line[1];
		PressData[i] = line[2];
		MaxTemp=Math.max(MaxTemp,TempData[i]);
		MaxPress=Math.max(MaxPress,PressData[i]);
		MaxVal=Math.max(MaxPress,MaxTemp);
	}
	//set y axis division
	var yDiv = Math.floor(myChart.height/MaxVal);
	//set label division
	roundFactor=Math.round(MaxVal/10);
	yLabelDiv=Math.round(myChart.height/(yDiv*TextSize*roundFactor))*roundFactor;
	//draw y axis and horizontal grid
	for (i=0;i<=myChart.height;i++)	{
	if (i % yLabelDiv == 0) {
	myChart_context.fillText(i, yLabelPos,myChart.height-i*yDiv); 
	myChart_context.beginPath();
	myChart_context.moveTo(yLabelPos, myChart.height-i*yDiv);
	myChart_context.lineTo(myChart.width, myChart.height-i*yDiv);
	myChart_context.lineWidth=0.5;
	myChart_context.stroke();
	}
	}
	//draw time axis and first line graph
	myChart_context.beginPath();
	myChart_context.moveTo(0, myChart.height-TempData[0]*yDiv);
	for (i=1;i<lines.length-1;i++)	{	
	if (i ==2 || i % xDiv ==0){ myChart_context.fillText(TimeAxis[i], i*xDiv,xLabelHeight);}
	myChart_context.lineTo(i*xDiv, myChart.height-TempData[i]*yDiv);
	}
	myChart_context.lineWidth=1;
	myChart_context.strokeStyle = '#ff0000';
	myChart_context.stroke();
	//second line graph
	myChart_context.beginPath();
	myChart_context.moveTo(0, myChart.height-PressData[0]*yDiv);
	for (i=1;i<lines.length-1;i++)	{	
	myChart_context.lineTo(i*xDiv, myChart.height-PressData[i]*yDiv);
	}
	myChart_context.lineWidth=1;
	myChart_context.strokeStyle = '#00ff00';
	myChart_context.stroke();
	myChart_context.fillStyle = '#00ff00';
	myChart_context.fillText("Pressure",myChart.width-12*TextSize,myChart.height-yLabelPos*2);
	myChart_context.fillStyle = '#ff0000';
	myChart_context.fillText("Temperature",myChart.width-7*TextSize,myChart.height-yLabelPos*2);
	myChart_context.fillStyle = '#000000';
	myChart_context.strokeStyle = '#000000';
	});
}
function checkMode()
{
	for (var i=0; i < document.InputData.FormSetMode.length;i++){
		if (document.InputData.FormSetMode[i].checked) {SetMode=document.InputData.FormSetMode[i].value;}
	}
	if (SetMode==11 || SetMode==21) document.getElementById("warning").innerHTML="Needs a ";
	else document.getElementById("warning").innerHTML="";
}

function ProgressBar(OuterId,InnerId,Value,MaxValue,Width) {
	//alert("scaling mode"+Value+" "+MaxValue);
	if (Value>0 && MaxValue>0){
		var percent=Math.round(Value*100/MaxValue);
		Value=Math.round(Value/MaxValue*Width);
		Value=Math.min(Value,Width);
		//alert("scaling mode");
		//Width=Width+5;
		document.getElementById(OuterId).style.width=Width+"px";
		document.getElementById(InnerId).innerHTML="&nbsp;"+percent+"%";
		document.getElementById(InnerId).style.width=Value+"px";
		document.getElementById(OuterId).style.visibility="visible";
		document.getElementById(InnerId).style.visibility="visible";
		}
	else {
		document.getElementById(OuterId).style.visibility="hidden";
		document.getElementById(InnerId).style.visibility="hidden";
	}
}

function UpdateSetValues()
{
	document.InputData.SetTbot.value=document.InputData.TempSlider.value;
	
	for (var i=0; i < document.InputData.PressRadio.length;i++){
		if (document.InputData.PressRadio[i].checked) {RadioPress=document.InputData.PressRadio[i].value;}
	}
	document.InputData.SetPress.value=RadioPress;
}

function CookProgress(){
	remainingtime=TotalSec-elapsedtime;
	if (remainingtime>0) {
		elapsedtime=elapsedtime+SecPerPixel;
		barwidth++;
	}
	remaining=HumanTime(remainingtime);
	elapsed=HumanTime(elapsedtime);
}

function LeadingZero(Value){
	if (Value<10) Value="0"+Value;
	return Value;
}

function HumanTime(InputTime){
	Hours=Math.floor(InputTime/(60*60));
	Minutes=Math.floor(InputTime/60-Hours*60);
	Seconds=Math.round(InputTime-Minutes*60-Hours*60*60);
	if (Hours>0) TimeString ='' +Hours+' h '+Minutes+' m '+Seconds+' s'; 
	else if (Seconds>0) TimeString =''+Minutes+' m '+Seconds+' s';
	else TimeString = Seconds+' s';
	return TimeString;
}


function SendCommand(){
	LastGrams=0;	
	var xmlhttp2;
	if (window.XMLHttpRequest)  {  xmlhttp2=new XMLHttpRequest();  }
	else  {  xmlhttp2=new ActiveXObject("Microsoft.XMLHTTP");  }
	//var ind =document.InputData.SetMode.selectedIndex;
	//var SetMode= document.InputData.SetMode.checked.value;
	//alert(document.InputData.FormSetMode[3].value);
	SetMode=0;
	for (var i=0; i < document.InputData.FormSetMode.length;i++){
		if (document.InputData.FormSetMode[i].checked) {SetMode=document.InputData.FormSetMode[i].value;}
	//	else {SetMode=0;}
	}
	SetTime=document.InputData.SetTimeH.value*60*60 + document.InputData.SetTimeM.value*60 + document.InputData.SetTimeS.value*1;
	var command="{\"T0\":" + document.InputData.SetTbot.value + ",\"P0\":" + document.InputData.SetPress.value+",\"M0RPM\":"  + document.InputData.SetRpm.value + ",\"M0ON\":" +document.InputData.SetRun.value + ",\"M0OFF\":" + document.InputData.SetPause.value + ",\"W0\":" + document.InputData.SetGrams.value + ",\"STIME\":" + SetTime + ",\"SMODE\":" + SetMode + ",\"SID\":" + document.InputData.StepID.value + "}";
	document.InputData.StepID.value = parseInt(document.InputData.StepID.value)+1;
	//alert("Debug:" + command);
	xmlhttp2.open("GET","functions/sendcommand.php?command="+command,true);
	xmlhttp2.send();
}

function ChangeGraphInterval(){
	GraphUpdateTimeout=document.InputData.SetGraphUpdate.value;
	clearInterval(updateGraph);
	GraphUpdateTimeout=1000*GraphUpdateTimeout;
	//alert(GraphUpdateTimeout);
	updateGraph = window.setInterval("MakeGraph()",GraphUpdateTimeout);
}

function StopNow(){
	var xmlhttp2;
	if (window.XMLHttpRequest)  {  xmlhttp2=new XMLHttpRequest();  }
	else  {  xmlhttp2=new ActiveXObject("Microsoft.XMLHTTP");  }
	var command="{\"T0\":0,\"P0\":0,\"M0RPM\":0,\"M0ON\":0,\"M0OFF\":0,\"W0\":0,\"STIME\":0,\"SMODE\":0,\"SID\":-1}";
	document.InputData.StepID.value = 0;
	//alert("Debug:" + command);
	xmlhttp2.open("GET","functions/sendcommand.php?command="+command,true);
	xmlhttp2.send();	
}
