/**********************************MMTPFORMAT*********************************
 0-15 : \r\n                                        2bytes
 16-47 : mmtp  										4bytes
 48-49   : type  0:txt 1:img 2:video
 50   : is first 0: false 1: true
 51-63: blank aligin 								2bytes	
 64-95:	reserve     								4bytes										 
 96-127:content length								4bytes 
 128-160:optionis length 							4bytes  
 content  .... options
 //// options always are c string ended with '\0'
**************************************end*************************************/
