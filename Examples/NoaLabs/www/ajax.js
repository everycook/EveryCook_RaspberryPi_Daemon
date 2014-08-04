/*
Try
*/
function getfile() {
	var xmlhttp;
	var newstring;
	id = new Array('id0', 'id1', 'id2', 'id3', 'id4', 'id5', 
		       'id6', 'id7', 'id8', 'id9', 'id10', 'id11');
	if (window.XMLHttpRequest)
	{
	xmlhttp = new XMLHttpRequest();
	}
	else
	{
	xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	xmlhttp.onreadystatechange=function()
	{
		if (xmlhttp.readyState==4 && xmlhttp.status==200)
		{
		newstring=xmlhttp.responseText;
		ch = new Array;
		ch = newstring.split("\n");
		for (var i = 0;i < id.length;i++)
		{
			chh = new Array;
			chh = ch[i].split("	");
			document.getElementById(id[i]).innerHTML = chh[1];
		}
		}
	}
	xmlhttp.open("GET", "readfile.txt", true);
	xmlhttp.send();
}

var xmlHttp1;  
function createXmlHttp()  
{  
	if(window.ActiveXObject)  
		xmlHttp1=new ActiveXObject("Microsoft.XMLHTTP");  
	else  
		xmlHttp1=new XMLHttpRequest();  
}  
function startRequest()  
{  
	try  
	{  
		createXmlHttp();    
		var url = "php1.php";
		var postedData=getRequestBody(document.forms["myForm"]);  

		xmlHttp1.open("post",url,true);  
		xmlHttp1.setRequestHeader("content-length",postedData.length); 
		xmlHttp1.setRequestHeader("content-type","application/x-www-form-urlencoded"); 
		xmlHttp1.onreadystatechange =onComplete;  
		xmlHttp1.send(postedData);  
	}  
	catch(e)  
	{  
		alert(e.message);  
	}   
}  
function onComplete()  
{  
	if(xmlHttp1.readyState==4&&xmlHttp1.status==200)  
	{   
//		document.getElementById("divResult").innerText=xmlHttp.responseText;  
	}  
}   
function getRequestBody(oForm)  
{  
	var aParams=new Array();  
	for(var i=0;i<oForm.elements.length;i++)  
	{  
		var sParam=encodeURIComponent(oForm.elements[i].name)  
		sParam+="=";  
		sParam+=encodeURIComponent(oForm.elements[i].value);  
		aParams.push(sParam);  
	}  
	return aParams.join("&");  
}
setInterval("getfile()", 100);
