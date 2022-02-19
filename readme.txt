what do you have to check and configure in the arduino sketch:
Please check & update before you upload the sketch to your arduino.
youtube channel containing a set of videos about this vendor machine:
https://youtube.com/playlist?list=PLrYKRosgwf_19UN1uPkxFnaKNIluaASAk

#################################################################################
Depending on selected board (e.g. UNO or Mega 2560) and number of buttons=selector and compartments the pin definition must be adapted as follows.
Please ensure that the cable connectors are established accordingly.
//uno
//const int selector[5] = {17, 16, 15, 14, 0 }; // input pins for selector buttons (A0=14, A1=15, A2=16, A3=17, Dig0=0) UNO
// when you want to user 2 * 5 compartments with UNO it is not possible to have power-off features at the same time because of lack of pins. In this case I suggest
// to run 1 row with 5 and one row with 4 compartements; use the free PIN to steer the power off relais; as an alternative you can consider to use PIN 1 (which is not recommended because of issues with serial-IOs); this may be possible 
// if you set all lines dealing with "Serial..." to "// serial..."
//mega
//const int selector[15] = {A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15}; // input pins for selector buttons MEGA
const int selector[5] = {A1, A2, A3, A4, A5}; // input pins for selector buttons MEGA 5 buttons and 5/10 or 15 boxes


for Uno with 5 compartments you can use more features than  having 10 compartments; 
to activate search for "refillbutton", "homematic" and "poweroff"; remove "//" in these areas


##################################################################################
Following row defines the price for each compartment. In this example it is 5€; Having 5 Buttons at the vendor machine even with 10 or 15 compartments the number of price is fix: 5
In dieser Zeile kann man vorgeben, welcher Preis für die Fächer vorbelegt werden soll (in diesem Beispiel 5€; es müssen immmer 5 Werte sein, auch wenn man 10 Fächer hat):

int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default 500ct (eeprom currently not used)

How many compartments/rows are available?
Wieviele Fächer je Reihe werden angeschlossen? 

bei 5 Fächern / 5 compartments:
int conveyorItems[5] = {1, 1, 1, 1, 1};
const int maxrow = 1;
const int max = 5; 
int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default value in ct

bei 10 Fächern / 10 compartments:
int conveyorItems[5] = {2, 2, 2, 2, 2};
const int maxrow = 2;
const int max = 5; 
int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default value in ct


bei 15 Fächern / 15 compartments:
int conveyorItems[5] = {3, 3, 3, 3, 3};
const int maxrow = 3;
const int max = 5; 
int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default value in ct

If you want to run the system with a button for each compartment number of rows should be set to 1

##################################################################################
new feature with release 2 - powersave:
after reaching idlePeriod relais "poweroffrelais" cuts the power; wakeup with poweron button (you need a separate self holding relais to keep power). See fritizing-documentation
This feature is active if powersave is set = 1 (otherwhise 0);
##################################################################################
new feature with release 2 - write/read eeprom
current price and remaining products in compartments per button-column will be stored in eeprom and is available after system restart. 
existing credit is deleted when restarting system
To force an eeprom update in case of new structure (e.g. additional set compartments to be activated)
##################################################################################
new feature with release 2 - reset credit after cutofftime = idle threshold
remaining credit will be deleted after a while
##################################################################################


any questions? pls contact me: mailto:honigautomat@gmx.de
Bei Fragen helfe ich gern weiter: honigautomat@gmx.de





