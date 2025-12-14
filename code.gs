// --- CONFIGURATION ---
var SHEET_ID = "1q5GYDQ71AqZzm48qTZml6X0Do6VyWLL-1N4_MGuoT5I"; // Paste your Sheet ID here
var AUTH_SHEET_NAME = "AuthorizedStudents";
var LOG_SHEET_NAME = "Attendance";
var MANUAL_PASSCODE = "1234"; // Passcode for students without cards entering ID manually

function doGet(e) {
  var ss = SpreadsheetApp.openById(SHEET_ID);
  var authSheet = ss.getSheetByName(AUTH_SHEET_NAME);
  var logSheet = ss.getSheetByName(LOG_SHEET_NAME);
  
  // Create sheets if they don't exist
  if (!authSheet) {
    authSheet = ss.insertSheet(AUTH_SHEET_NAME);
    // Header changed to StudentID
    authSheet.appendRow(["UID", "StudentID", "Name", "DateRegistered"]);
  }
  if (!logSheet) {
    logSheet = ss.insertSheet(LOG_SHEET_NAME);
    logSheet.appendRow(["Date", "Time", "UID/ID", "Name", "Method"]);
  }

  var action = e.parameter.action;
  
  // --- ACTION: CHECK AUTHORIZATION ---
  if (action == "check") {
    var uid = e.parameter.uid;
    var data = authSheet.getDataRange().getValues();
    
    for (var i = 1; i < data.length; i++) {
      if (String(data[i][0]) == String(uid)) { // Check UID column
        return ContentService.createTextOutput("authorized," + data[i][2]); // Return Name
      }
    }
    return ContentService.createTextOutput("unauthorized");
  }

  // --- ACTION: LOG ATTENDANCE (RFID) ---
  else if (action == "log") {
    var uid = e.parameter.uid;
    var name = e.parameter.name; 
    
    // If name not sent, find it
    if (!name) {
       var data = authSheet.getDataRange().getValues();
       for (var i = 1; i < data.length; i++) {
         if (String(data[i][0]) == String(uid)) {
           name = data[i][2];
           break;
         }
       }
    }
    
    var date = new Date();
    logSheet.appendRow([date.toLocaleDateString(), date.toLocaleTimeString(), uid, name, "RFID"]);
    return ContentService.createTextOutput("logged");
  }

  // --- ACTION: REGISTER NEW STUDENT ---
  else if (action == "register") {
    var uid = e.parameter.uid;
    var studentId = e.parameter.id; // Receiving student ID
    var name = e.parameter.name;
    
    // Check if already exists
    var data = authSheet.getDataRange().getValues();
    for (var i = 1; i < data.length; i++) {
      if (String(data[i][0]) == String(uid)) {
        return ContentService.createTextOutput("already_registered");
      }
    }
    
    authSheet.appendRow([uid, studentId, name, new Date()]);
    return ContentService.createTextOutput("registered");
  }
  
  // --- ACTION: MANUAL ENTRY (No Card) ---
  else if (action == "manual") {
    var pass = e.parameter.pass;
    var studentId = e.parameter.id;
    
    if (pass != MANUAL_PASSCODE) {
      return ContentService.createTextOutput("wrong_pass");
    }
    
    // Look up name based on Student ID in Auth Sheet
    var name = "Unknown";
    var data = authSheet.getDataRange().getValues();
    for (var i = 1; i < data.length; i++) {
      if (String(data[i][1]) == String(studentId)) { // Checking StudentID column (Column B)
        name = data[i][2];
        break;
      }
    }
    
    var date = new Date();
    logSheet.appendRow([date.toLocaleDateString(), date.toLocaleTimeString(), studentId, name, "Keypad"]);
    return ContentService.createTextOutput("logged_manual");
  }

  return ContentService.createTextOutput("error_unknown_action");
}
