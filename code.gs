function doGet(e) {
  //console.log("rfid!");
  var run = (e.parameter.page);
  Logger.log(JSON.stringify(e));
  if(run == "permissions")
    return getPermisions(e);
  if(run == "access")
    return postEntry(e);
  if(run == "error")
    return postError(e);
}

function postEntry(e){
  var sh = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Accesses");
  var lRow = sh.getLastRow(); 
  var lCol = sh.getLastColumn();
  var range = sh.getRange(3, 1, 1,lCol);
  sh.insertRowAfter(1);
  range.copyTo(sh.getRange(2, 1, 1, lCol), {contentsOnly:false});
  sh.getRange(2, 1, 1, 2).setValues([[new Date(), e.parameter.id]]);
  Logger.log(e.parameter.id);
  return HtmlService.createHtmlOutput(new Date());
}

function postError(e){
  var sh = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Log");
  var lRow = sh.getLastRow(); 
  var lCol = sh.getLastColumn();
  var range = sh.getRange(3,1,1,lCol);
  sh.insertRowAfter(1);
  range.copyTo(sh.getRange(2, 1, 1, lCol), {contentsOnly:false});
  sh.getRange(2, 1, 1, 2).setValues([[new Date(), e.parameter.id]]);
  sh.getRange(2, 4).setValue("Access Not Authorized");
  return HtmlService.createHtmlOutput(new Date());
}

function getPermisions(e) {
 
 var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Permissions");
  var data = sheet.getDataRange().getValues();
  var html = "";
  var now = new Date();
  for (i = 2; i < data.length; i++)
  {
    if( data[i][3] <= now) {//
      if(data[i][4] >= now) {//
        // console.log(data[i][0]);
        html += data[i][0] + ",";
      }
    }
  }
   sheet.getRange(1, 2).setValue(new Date());
  return ContentService.createTextOutput(html);
 //  Logger.log(html)
 // console.log(html);

}
