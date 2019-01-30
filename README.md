This is the calibration program to be used with the MMDVM and MMDVM_HS. It is started by  
adding the serial port onto the command line. On Windows the serial port must be  
prefixed with \\.\ as in \\.\COM4 to be valid. Once started the program takes  
the following commands:  

- MMDVM:

<table>
<tr><th>Command</th><th>Description</th></tr>
<tr><td>H/h</td><td>Display help</td></tr>
<tr><td>Q/q</td><td>Quit</td></tr>
<tr><td>W/w</td><td>Enable/disable modem debug messages</td></tr>
<tr><td>I</td><td>Toggle transmit inversion</td><tr>
<tr><td>i</td><td>Toggle receive inversion</td></tr>
<tr><td>O</td><td>Increase TX DC offset level</td></tr>
<tr><td>o</td><td>Decrease TX DC offset level</td></tr>
<tr><td>C</td><td>Increase RX DC offset level</td></tr>
<tr><td>c</td><td>Decrease RX DC offset level</td></tr>
<tr><td>P/p</td><td>Toggle PTT inversion</td></tr>
<tr><td>R</td><td>Increase receive level</td></tr>
<tr><td>r</td><td>Decrease receive level</td></tr>
<tr><td>T</td><td>Increase transmit level</td></tr>
<tr><td>t</td><td>Decrease transmit level</td></tr>
<tr><td>d</td><td>D-Star mode</td></tr>
<tr><td>D</td><td>Set DMR Deviation Mode. Generates a 1.2Khz Sinewave. Set radio for 2.75 Khz Deviation</td></tr>
<tr><td>L/l</td><td>DMR Low Frequency Mode (80 Hz square wave)</td></tr>
<tr><td>A</td><td>DMR Duplex 1031 Hz Test Pattern (TS2 CC1 ID1 TG9)</td></tr>
<tr><td>M/m</td><td>DMR Simplex 1031 Hz Test Pattern (CC1 ID1 TG9)</td></tr>
<tr><td>a</td><td>P25 1011 Hz Test Pattern (NAC293 ID1 TG1)</td></tr>
<tr><td>N</td><td>NXDN 1031 Hz Test Pattern (RAN1 ID1 TG1)</td></tr>
<tr><td>K/k</td><td>BER Test Mode (FEC) for D-Star</td></tr>
<tr><td>b</td><td>BER Test Mode (FEC) for DMR Simplex (CC1)</td></tr>
<tr><td>B</td><td>BER Test Mode (1031 Hz Test Pattern) for DMR Simplex (CC1 ID1 TG9)</td></tr>
<tr><td>J</td><td>BER Test Mode (FEC) for YSF</td></tr>
<tr><td>j</td><td>BER Test Mode (FEC) for P25</td></tr>
<tr><td>n</td><td>BER Test Mode (FEC) for NXDN</td></tr>
<tr><td>g</td><td>POCSAG 600Hz Test Pattern</td></tr>
<tr><td>S/s</td><td>RSSI Mode</td></tr>
<tr><td>V/v</td><td>Display version of MMDVMCal</td></tr>
<tr><td>&lt;space&gt;</td><td>Toggle transmit</td></tr>
</table>

- MMDVM_HS:

<table>
<tr><th>Command</th><th>Description</th></tr>
<tr><td>H/h</td><td>Display help</td></tr>
<tr><td>Q/q</td><td>Quit</td></tr>
<tr><td>W/w</td><td>Enable/disable modem debug messages</td></tr>
<tr><td>E/e</td><td>Enter frequency</td><tr>
<tr><td>F</td><td>Increase frequency</td></tr>
<tr><td>f</td><td>Decrease frequency</td></tr>
<tr><td>Z/z</td><td>Enter frequency step</td></tr>
<tr><td>T</td><td>Increase deviation</td></tr>
<tr><td>t</td><td>Decrease deviation</td></tr>
<tr><td>P</td><td>Increase RF power</td></tr>
<tr><td>p</td><td>Decrease RF power</td></tr>
<tr><td>C/c</td><td>Carrier Only Mode</td></tr>
<tr><td>D/d</td><td>DMR Deviation Mode</td></tr>
<tr><td>M/m</td><td>DMR Simplex 1031 Hz Test Pattern (CC1 ID1 TG9)</td></tr>
<tr><td>K/k</td><td>BER Test Mode (FEC) for D-Star</td></tr>
<tr><td>b</td><td>BER Test Mode (FEC) for DMR Simplex (CC1)</td></tr>
<tr><td>B</td><td>BER Test Mode (1031 Hz Test Pattern) for DMR Simplex (CC1 ID1 TG9)</td></tr>
<tr><td>J</td><td>BER Test Mode (FEC) for YSF</td></tr>
<tr><td>j</td><td>BER Test Mode (FEC) for P25</td></tr>
<tr><td>n</td><td>BER Test Mode (FEC) for NXDN</td></tr>
<tr><td>g</td><td>POCSAG 600Hz Test Pattern</td></tr>
<tr><td>S/s</td><td>RSSI Mode</td></tr>
<tr><td>I/i</td><td>Interrupt Counter Mode</td></tr>
<tr><td>V/v</td><td>Display version of MMDVMCal</td></tr>
<tr><td>&lt;space&gt;</td><td>Toggle transmit</td></tr>
</table>
