READ ME FIRST

Rulare: 
In linux, deschidem 2 terminale diferite.
Pe unul rulam comanda "make" si apoi ./server <port> users_data_file, de ex: ./server 12345 users_data_file
Pe al doilea rulam ./client <ip_server> <port>, de ex: ./client 127.127.0.1 12345.

Datele legate de utilizatori le putem gasi in "users_data_file" in urmatoarea ordine (pe prima linie se gaseste numarul de persoane):
<nume> <prenume> <numar_card> <pin> <parola_secretă> <sold>

Comenzile se dau doar din consola clientului si acestea sunt urmatoarele:
login <numar_card> <pin_card>
logout
listsold
getmoney <suma_de_retras>
putmoney <suma_de_depus>
quit
