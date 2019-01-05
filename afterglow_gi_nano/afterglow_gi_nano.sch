EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "Afterglow GI"
Date ""
Rev "1.0"
Comp "morbid cornflakes"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L Regulator_Linear:L7805 U?
U 1 1 5C2F77B7
P 3900 1150
F 0 "U?" H 3900 1392 50  0000 C CNN
F 1 "L7805" H 3900 1301 50  0000 C CNN
F 2 "" H 3925 1000 50  0001 L CIN
F 3 "http://www.st.com/content/ccc/resource/technical/document/datasheet/41/4f/b3/b0/12/d4/47/88/CD00000444.pdf/files/CD00000444.pdf/jcr:content/translations/en.CD00000444.pdf" H 3900 1100 50  0001 C CNN
	1    3900 1150
	1    0    0    -1  
$EndComp
$Comp
L MCU_Module:Arduino_Nano_v3.x A?
U 1 1 5C2F7899
P 5100 3250
F 0 "A?" H 5100 2164 50  0000 C CNN
F 1 "Arduino_Nano_v3.x" H 4500 2250 50  0000 C CNN
F 2 "Module:Arduino_Nano" H 5250 2300 50  0001 L CNN
F 3 "http://www.mouser.com/pdfdocs/Gravitech_Arduino_Nano3_0.pdf" H 5100 2250 50  0001 C CNN
	1    5100 3250
	1    0    0    -1  
$EndComp
$Comp
L 74xx:74LS374 U?
U 1 1 5C2F7ADE
P 2900 3400
F 0 "U?" H 2550 4350 50  0000 C CNN
F 1 "74LS374" H 2550 4250 50  0000 C CNN
F 2 "" H 2900 3400 50  0001 C CNN
F 3 "http://www.ti.com/lit/gpn/sn74LS374" H 2900 3400 50  0001 C CNN
	1    2900 3400
	1    0    0    -1  
$EndComp
$Comp
L dk_Transistors-FETs-MOSFETs-Single:FQP30N06L Q?
U 1 1 5C2F7E46
P 7000 1950
F 0 "Q?" H 7107 2003 60  0000 L CNN
F 1 "FQP30N06L" H 7107 1897 60  0000 L CNN
F 2 "digikey-footprints:TO-220-3" H 7200 2150 60  0001 L CNN
F 3 "https://www.fairchildsemi.com/datasheets/FQ/FQP30N06L.pdf" H 7200 2250 60  0001 L CNN
F 4 "FQP30N06L-ND" H 7200 2350 60  0001 L CNN "Digi-Key_PN"
F 5 "FQP30N06L" H 7200 2450 60  0001 L CNN "MPN"
F 6 "Discrete Semiconductor Products" H 7200 2550 60  0001 L CNN "Category"
F 7 "Transistors - FETs, MOSFETs - Single" H 7200 2650 60  0001 L CNN "Family"
F 8 "https://www.fairchildsemi.com/datasheets/FQ/FQP30N06L.pdf" H 7200 2750 60  0001 L CNN "DK_Datasheet_Link"
F 9 "/product-detail/en/on-semiconductor/FQP30N06L/FQP30N06L-ND/1055122" H 7200 2850 60  0001 L CNN "DK_Detail_Page"
F 10 "MOSFET N-CH 60V 32A TO-220" H 7200 2950 60  0001 L CNN "Description"
F 11 "ON Semiconductor" H 7200 3050 60  0001 L CNN "Manufacturer"
F 12 "Active" H 7200 3150 60  0001 L CNN "Status"
	1    7000 1950
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_02x17_Odd_Even J?
U 1 1 5C2F80E8
P 1300 2800
F 0 "J?" H 1350 1775 50  0000 C CNN
F 1 "Conn_02x17_Odd_Even" H 1350 1866 50  0000 C CNN
F 2 "" H 1300 2800 50  0001 C CNN
F 3 "~" H 1300 2800 50  0001 C CNN
	1    1300 2800
	-1   0    0    1   
$EndComp
$Comp
L Device:R_POT_TRIM RV?
U 1 1 5C2F862F
P 6150 3350
F 0 "RV?" H 6081 3396 50  0000 R CNN
F 1 "R_POT_TRIM" H 6081 3305 50  0000 R CNN
F 2 "" H 6150 3350 50  0001 C CNN
F 3 "~" H 6150 3350 50  0001 C CNN
	1    6150 3350
	-1   0    0    -1  
$EndComp
$Comp
L Connector:Conn_01x02_Female J?
U 1 1 5C2F87F9
P 3000 1250
F 0 "J?" H 2894 925 50  0000 C CNN
F 1 "Conn_01x02_Female" H 2894 1016 50  0000 C CNN
F 2 "" H 3000 1250 50  0001 C CNN
F 3 "~" H 3000 1250 50  0001 C CNN
	1    3000 1250
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x11_Male J?
U 1 1 5C2F89CF
P 8600 1950
F 0 "J?" H 8573 1880 50  0000 R CNN
F 1 "Conn_01x11_Male" H 8573 1971 50  0000 R CNN
F 2 "" H 8600 1950 50  0001 C CNN
F 3 "~" H 8600 1950 50  0001 C CNN
	1    8600 1950
	-1   0    0    1   
$EndComp
$Comp
L Connector:Conn_01x11_Male J?
U 1 1 5C2F8A6B
P 9750 1950
F 0 "J?" H 9723 1880 50  0000 R CNN
F 1 "Conn_01x11_Male" H 9723 1971 50  0000 R CNN
F 2 "" H 9750 1950 50  0001 C CNN
F 3 "~" H 9750 1950 50  0001 C CNN
	1    9750 1950
	-1   0    0    1   
$EndComp
$Comp
L Device:LED D?
U 1 1 5C2F8DB2
P 7350 3550
F 0 "D?" V 7388 3433 50  0000 R CNN
F 1 "LED" V 7297 3433 50  0000 R CNN
F 2 "" H 7350 3550 50  0001 C CNN
F 3 "~" H 7350 3550 50  0001 C CNN
	1    7350 3550
	0    -1   -1   0   
$EndComp
$Comp
L Device:R R?
U 1 1 5C2F8FAC
P 6650 4000
F 0 "R?" H 6720 4046 50  0000 L CNN
F 1 "R" H 6720 3955 50  0000 L CNN
F 2 "" V 6580 4000 50  0001 C CNN
F 3 "~" H 6650 4000 50  0001 C CNN
	1    6650 4000
	1    0    0    -1  
$EndComp
$Comp
L Device:Fuse F?
U 1 1 5C2F922F
P 3600 5050
F 0 "F?" V 3403 5050 50  0000 C CNN
F 1 "Fuse" V 3494 5050 50  0000 C CNN
F 2 "" V 3530 5050 50  0001 C CNN
F 3 "~" H 3600 5050 50  0001 C CNN
	1    3600 5050
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR?
U 1 1 5C2F945E
P 6150 3500
F 0 "#PWR?" H 6150 3250 50  0001 C CNN
F 1 "GND" H 6155 3327 50  0000 C CNN
F 2 "" H 6150 3500 50  0001 C CNN
F 3 "" H 6150 3500 50  0001 C CNN
	1    6150 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	5600 3350 6000 3350
Wire Wire Line
	6150 3200 6150 2150
Wire Wire Line
	6150 2150 5300 2150
Wire Wire Line
	5300 2150 5300 2250
NoConn ~ 5200 2250
NoConn ~ 5000 2250
NoConn ~ 5600 3450
NoConn ~ 5600 3550
NoConn ~ 5600 3650
NoConn ~ 5600 3750
NoConn ~ 5600 3850
NoConn ~ 5600 3950
NoConn ~ 4600 2650
NoConn ~ 4600 2750
NoConn ~ 5600 3050
Wire Wire Line
	5100 4250 5100 4400
Wire Wire Line
	5100 4400 5150 4400
Wire Wire Line
	5200 4400 5200 4250
$Comp
L power:GND #PWR?
U 1 1 5C2FA2D8
P 5150 4400
F 0 "#PWR?" H 5150 4150 50  0001 C CNN
F 1 "GND" H 5155 4227 50  0000 C CNN
F 2 "" H 5150 4400 50  0001 C CNN
F 3 "" H 5150 4400 50  0001 C CNN
	1    5150 4400
	1    0    0    -1  
$EndComp
Connection ~ 5150 4400
Wire Wire Line
	5150 4400 5200 4400
NoConn ~ 5600 2650
NoConn ~ 5600 2750
$Comp
L power:GND #PWR?
U 1 1 5C2FA697
P 2950 4200
F 0 "#PWR?" H 2950 3950 50  0001 C CNN
F 1 "GND" H 2955 4027 50  0000 C CNN
F 2 "" H 2950 4200 50  0001 C CNN
F 3 "" H 2950 4200 50  0001 C CNN
	1    2950 4200
	1    0    0    -1  
$EndComp
$Comp
L power:+12V #PWR?
U 1 1 5C2FAA66
P 3350 900
F 0 "#PWR?" H 3350 750 50  0001 C CNN
F 1 "+12V" H 3365 1073 50  0000 C CNN
F 2 "" H 3350 900 50  0001 C CNN
F 3 "" H 3350 900 50  0001 C CNN
	1    3350 900 
	1    0    0    -1  
$EndComp
Wire Wire Line
	3200 1150 3350 1150
Connection ~ 3350 1150
Wire Wire Line
	3350 1150 3500 1150
Wire Wire Line
	3350 1150 3350 900 
Wire Wire Line
	3200 1250 3350 1250
Wire Wire Line
	3350 1250 3350 1650
Wire Wire Line
	3350 1650 3500 1650
Wire Wire Line
	3900 1650 3900 1450
$Comp
L power:GND #PWR?
U 1 1 5C2FB4D5
P 3900 1650
F 0 "#PWR?" H 3900 1400 50  0001 C CNN
F 1 "GND" H 3905 1477 50  0000 C CNN
F 2 "" H 3900 1650 50  0001 C CNN
F 3 "" H 3900 1650 50  0001 C CNN
	1    3900 1650
	1    0    0    -1  
$EndComp
Connection ~ 3900 1650
Wire Wire Line
	5300 2150 5300 2000
Connection ~ 5300 2150
Text Label 5300 2000 0    50   ~ 0
5V
$Comp
L Device:CP_Small C?
U 1 1 5C2FC118
P 3500 1400
F 0 "C?" H 3588 1446 50  0000 L CNN
F 1 "CP_Small" H 3588 1355 50  0000 L CNN
F 2 "" H 3500 1400 50  0001 C CNN
F 3 "~" H 3500 1400 50  0001 C CNN
	1    3500 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	3500 1300 3500 1150
Connection ~ 3500 1150
Wire Wire Line
	3500 1150 3600 1150
Wire Wire Line
	3500 1500 3500 1650
Connection ~ 3500 1650
Wire Wire Line
	3500 1650 3900 1650
Wire Wire Line
	4200 1150 4350 1150
Wire Wire Line
	3900 1650 4350 1650
Wire Wire Line
	4350 1650 4350 1500
$Comp
L Device:C_Small C?
U 1 1 5C2FC91D
P 4350 1400
F 0 "C?" H 4442 1446 50  0000 L CNN
F 1 "C_Small" H 4442 1355 50  0000 L CNN
F 2 "" H 4350 1400 50  0001 C CNN
F 3 "~" H 4350 1400 50  0001 C CNN
	1    4350 1400
	1    0    0    -1  
$EndComp
Wire Wire Line
	4350 1150 4350 1300
Connection ~ 4350 1150
Wire Wire Line
	4350 1150 4700 1150
Text Label 4700 1150 0    50   ~ 0
5V
$Comp
L Device:C_Small C?
U 1 1 5C2FCDEF
P 3250 2550
F 0 "C?" V 3021 2550 50  0000 C CNN
F 1 "C_Small" V 3112 2550 50  0000 C CNN
F 2 "" H 3250 2550 50  0001 C CNN
F 3 "~" H 3250 2550 50  0001 C CNN
	1    3250 2550
	0    1    1    0   
$EndComp
Wire Wire Line
	3350 2550 3550 2550
Wire Wire Line
	3550 2550 3550 2650
$Comp
L power:GND #PWR?
U 1 1 5C2FD2C9
P 3550 2650
F 0 "#PWR?" H 3550 2400 50  0001 C CNN
F 1 "GND" H 3555 2477 50  0000 C CNN
F 2 "" H 3550 2650 50  0001 C CNN
F 3 "" H 3550 2650 50  0001 C CNN
	1    3550 2650
	1    0    0    -1  
$EndComp
Wire Wire Line
	3150 2550 2900 2550
Wire Wire Line
	2900 2550 2900 2600
Wire Wire Line
	2900 2550 2900 2450
Connection ~ 2900 2550
Text Label 2900 2450 0    50   ~ 0
5V
$Comp
L Connector:Conn_01x03_Male J?
U 1 1 5C2FFDF1
P 8600 2850
F 0 "J?" H 8573 2780 50  0000 R CNN
F 1 "Conn_01x03_Male" H 8573 2871 50  0000 R CNN
F 2 "" H 8600 2850 50  0001 C CNN
F 3 "~" H 8600 2850 50  0001 C CNN
	1    8600 2850
	-1   0    0    1   
$EndComp
$EndSCHEMATC
