/**********************************MMTPFORMAT*********************************
 0-1 : \r\n
 2-5 : mmtp
 6   : type  0:txt 1:img 2:video
 7	 : is first 0: false 1: true
 8-15: blank aligin
 16-31:reserve 
 32-63:content length 

 if contents contains \r\nmmtp replace with \r\nmmtp
**************************************end*************************************/
