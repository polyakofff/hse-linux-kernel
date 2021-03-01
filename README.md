#### Последовательность действий:
```
1. "заходим в директорию с phonebook.c"
2. make
3. insmod phonebook.ko
4. "смотрим в kern.log и видим "Команда mknod /dev/phonebook c x 0 для создания файла устройства""
5. mknod /dev/phonebook c x 0
6. "взаимодействуем"
7. rm /dev/phonebook
8. rmmod phonebook
```


#### Команды:
```
+name=string;age=int;number=long;email=string;  /*добавить контакт (имя обязательно)*/
-name=string;                                   /*удалить контакт по имени*/
?name=string;                                   /*найти контакт по имени*/
```


#### Пример взаимодействия:
```
$ echo "+name=Bob;age=33;number=1234567891011;email=emo" > /dev/phonebook
$ echo "?name=Bob;" > /dev/phonebook
$ cat /dev/phonebook
name=Bob,age=33,number=1234567891011,email=emo
$ echo "+name=Lev Polyakov;age=19;" > /dev/phonebook
$ cat /dev/phonebook
$ echo "-name=Bob;" > /dev/phonebook
$ echo "?name=Bob;" > /dev/phonebook
$ cat /dev/phonebook
Не найдено
```
