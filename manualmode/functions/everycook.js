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

	var command="{\"T0\":" + document.InputData.SetTbot.value + ",\"P0\":" + document.InputData.SetPress.value+",\"M0RPM\":"  + document.InputData.SetRpm.value + ",\"M0ON\":" +document.InputData.SetRun.value + ",\"M0OFF\":" + document.InputData.SetPause.value + ",\"W0\":" + document.InputData.SetGrams.value + ",\"STIME\":" + document.InputData.SetTime.value + ",\"SMODE\":" + SetMode + ",\"SID\":" + document.InputData.StepID.value + "}";
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
