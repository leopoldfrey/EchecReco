
function anything()
{
	var a = arrayfromargs(messagename, arguments);
	post("received message " + a + " \n");
	var b = a.toString().split('.');
	outlet(0, parseInt(b[0]), parseInt(b[1]), parseInt(b[2]), parseInt(b[3]));
}