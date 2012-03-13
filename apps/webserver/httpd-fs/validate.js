function isIP(obj) {
var ary = obj.value.split("."); var ip = true;
for (var i in ary) {ip = (!ary[i].match(/^\d{1,3}$/) || (Number(ary[i]) > 255)) ? false : ip;}
ip = (ary.length != 4) ? false : ip;
if (!ip) { obj.style.background = "red"; return 0;
} else { obj.style.background = ""; return 1; }}
function isNumValid(str) { var nE=/^[0-9]+$/; if(str.match(nE)) return 1; else return 0; }
function isNumOk(obj, min, max) {
if (!isNumValid(obj.value) || obj.value < min || obj.value > max) {
obj.style.background = "red"; return 0; } else { obj.style.background = ""; return 1; } }
function isStrOk(obj, minlen, maxlen) {
var nE=/^\w+$/; if (!obj.value.match(nE) || obj.value.length > maxlen) {
obj.style.background="red"; return 0; } else { obj.style.background = "";return 1; } }
function isTime(obj) {
var result = false, m; var re = /^\s*([01]?\d|2[0-3]):?([0-5]\d)\s*$/;
if ((m = obj.value.match(re))) { result = (m[1].length == 2 ? "" : "0") + m[1] + ":" + m[2]; }
if (!result) { obj.style.background="red"; return 0; } else { obj.style.background = "";return 1; } }
function isDate(obj) {
 var regDate = '^(19[0-9]{2}|2[0-9]{3})-(0[1-9]{1}|1[0-2]{1}){1}-(0[1-9]|(1|2)[0-9]|3[0-1]){1}$';
 if (!obj.value.match(regDate)) { obj.style.background="red"; return 0; } else { obj.style.background = "";return 1; } }
function doAlert() { alert("One or more fields are incorrect, please correct and try again"); return false; }
