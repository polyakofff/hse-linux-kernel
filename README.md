Последовательность действий:
"заходим в директорию с phonebook.c"
-make
-insmod phonebook.ko
"смотрим в kern.log и видим "Команда mknod /dev/phonebook c x 0 для создания файла устройства""
-mknod /dev/phonebook c x 0
"взаимодействуем"
-rm /dev/phonebook
-rmmod phonebook


Команды:
+name=string;age=int;number=long;email=string;  /*добавить контакт (имя обязательно)*/
-name=string;                                   /*удалить контакт по имени*/
?name=string;                                   /*найти контакт по имени*/


Пример взаимодействия:
-echo "+name=Bob;age=33;number=1234567891011;email=@@@" > /dev/phonebook
-cat /dev/phonebook                                                       /**/
-echo "?name=Bob;" > /dev/phonebook
-cat /dev/phonebook                                                       /*name=Bob,age=33,number=1234567891011,email=@@@*/
-echo "+name=Lev Polyakov;age=19;" > /dev/phonebook
-cat /dev/phonebook                                                       /**/
-echo "-name=Bob;" > /dev/phonebook
-echo "?name=Bob;" > /dev/phonebook
-cat /dev/phonebook                                                       /*Не найдено*/
