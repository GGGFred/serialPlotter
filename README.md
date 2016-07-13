# serialPlotter

This application aims to be a simple adaptive serial port plotter, which means than you can plot up to 10 channels by sending
"%f %f %f .....\r\n" from any embedded device by USART port..

QCustomPlot was used to achieve the plot..
http://qcustomplot.com/

This is like the plotter integrated in Arduino IDE, but I'm looking for give more power.

-- By now the dump data it's not working yet, I want to dump data of the channels in a text file

I found a similar application in 
https://developer.mbed.org/users/borislav/notebook/serial-port-plotter/
But is limited to 3 channels

I hope this application helps you to achieve your target
