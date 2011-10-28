function setlight(IP, channel, level)
{
    var xmlHttp = null;
    level = level * 100;
    var theURL = "http://" + IP + "/cgi/set_level.cgi?channel=" + channel
+ "&level=" + level;

    xmlHttp = new XMLHttpRequest();
    xmlHttp.open( "GET", theURL, false );
    xmlHttp.send( null );
    return xmlHttp.responseText;
}

window.onload = function()
{
	new Dragdealer('slider_1', {
	  steps: 20,
	  animationCallback: function(x, y)
	  {
	    setlight("192.168.0.11", 0, x);
	  }
	});
	new Dragdealer('slider_2', {
	  steps: 20,
	  animationCallback: function(x, y)
	  {
	    setlight("192.168.0.11", 2, x);
	  }
	});
	new Dragdealer('slider_3', {
	  steps: 20,
	  animationCallback: function(x, y)
	  {
	    setlight("192.168.0.11", 3, x);
	  }
	});
	new Dragdealer('slider_4', {
	  steps: 20,
	  animationCallback: function(x, y)
	  {
	    setlight("192.168.0.11", 4, x);
	  }
	});

}

