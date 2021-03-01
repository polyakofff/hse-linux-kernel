#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/list.h>
// #include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
// #include <linux/uaccess.h> 

MODULE_LICENSE("GPL");

#define DEVICE_NAME "phonebook"
#define MAX_COMMAND_SIZE 1024
#define MAX_REPLY_SIZE 1024

static struct Contact {
	char *name;
	int age;
	long number;
	char *email;
	struct list_head list;
};
static struct list_head contacts;
static char command[MAX_COMMAND_SIZE];
static char reply[MAX_REPLY_SIZE];




static int open_now_counter = 0;

static int phonebook_open(struct inode *inode, struct file *file) {
	if (open_now_counter) {
		printk(KERN_INFO "Устройство уже открыто\n");
		return -EBUSY;
	}
	open_now_counter++;
	try_module_get(THIS_MODULE);

	return 0;
}

static int phonebook_release(struct inode *inode, struct file *file) {
	open_now_counter--;
	module_put(THIS_MODULE);

	return 0;
}

static ssize_t phonebook_read(struct file *file, char __user *buf, size_t size, loff_t *offset) {
	int left = min(MAX_REPLY_SIZE - *offset, size);
	if (left <= 0)
		return 0;
	if (copy_to_user(buf, reply + *offset, left)) {
		printk(KERN_INFO "Плохой адрес (read)\n");
		return -EFAULT;
	}
	*offset += left;
	return left;
}

static int retrieve_value_by_key(char *key, char **value) {
	char *start = strstr(command, key);
	if (!start)
		return -1;
	start += strlen(key);
	char *end = strchr(start, ';');
	if (!end)
		return -1;
	int size = end - start;
	*value = kmalloc(size, GFP_KERNEL);
	strncpy(*value, start, size);
	return 0;
}

static int add_contact(void) {
	int ret;
	struct Contact *con = kmalloc(sizeof(struct Contact), GFP_KERNEL);
	
	ret = retrieve_value_by_key("name=", &con->name);
	if (ret) {
		printk(KERN_INFO "Нет имени контакта\n");
		goto clean;
	}
	char *temp = NULL;
	ret = retrieve_value_by_key("age=", &temp);
	if (!ret) {
		ret = kstrtoint(temp, 10, &con->age);
		if (ret) {
			printk(KERN_INFO "Ошибка при парсинге возраста\n");
			goto clean;
		}
	}
	kfree(temp);
	ret = retrieve_value_by_key("number=", &temp);
	if (!ret) {
		ret = kstrtol(temp, 10, &con->number);
		if (ret) {
			printk(KERN_INFO "Ошибка при парсинге номера\n");
			goto clean;
		}
	}
	kfree(temp);
	ret = retrieve_value_by_key("email=", &con->email);
	
	INIT_LIST_HEAD(&con->list);
	list_add(&con->list, &contacts);
	
	return 0;
	
	clean:
	kfree(con->name);
	kfree(con->email);
	kfree(con);
	kfree(temp);
	
	return ret;
}

static int delete_contact(void) {
	int ret;
	
	char *name = NULL;
	ret = retrieve_value_by_key("name=", &name);
	if (ret) {
		printk(KERN_INFO "Нет имени контакта\n");
		goto clean;
	}
	
	bool found = false;
	struct list_head *i, *n;
	struct Contact *con;
	
	list_for_each_safe(i, n, &contacts) {
		con = list_entry(i, struct Contact, list);
		if (strcmp(con->name, name) == 0) {
			found = true;
			list_del(i);
			kfree(con);
			break;
		}
	}
	
	kfree(name);
	
	if (found) {
	
		return 0;
	} else {
		printk(KERN_INFO "Контакт не найден\n");
		return -1;
	}
	
	clean:
	kfree(name);
	
	return ret;
}

static void prepare_reply(struct Contact *con) {
	char *start = reply;
	int shift = sprintf(start, "name=%s,", con->name);
	start += shift;
	shift = sprintf(start, "age=%d,", con->age);
	start += shift;
	shift = sprintf(start, "number=%ld,", con->number);
	start += shift;
	shift = sprintf(start, "email=%s\n", con->email);
	start += shift;
}

static int find_contact(void) {
	int ret;
	
	char *name = NULL;
	ret = retrieve_value_by_key("name=", &name);
	if (ret) {
		printk(KERN_INFO "Нет имени контакта\n");
		goto clean;
	}
	
	bool found = false;
	struct list_head *i, *n;
	struct Contact *con;
	
	list_for_each_safe(i, n, &contacts) {
		con = list_entry(i, struct Contact, list);
		if (strcmp(con->name, name) == 0) {
			found = true;
			break;
		}
	}
	
	kfree(name);
	
	if (found) {
	
		prepare_reply(con);
		return 0;
	} else {
		printk(KERN_INFO "Контакт не найден\n");
		sprintf(reply, "Не найдено\n");
		return -1;
	}
	
	clean:
	kfree(name);
	
	return ret;
}

static int execute_command(void) {
	memset(reply, '\0', MAX_REPLY_SIZE);
	int ret;
	switch (command[0]) {
		case '+':
			ret = add_contact();
			if (!ret)
				printk(KERN_INFO "Контакт добавлен\n");
			break;
		case '-':
			ret = delete_contact();
			if (!ret)
				printk(KERN_INFO "Контакт удален\n");
			break;
		case '?':
			ret = find_contact();
			if (!ret)
				printk(KERN_INFO "Контакт найден\n");
			break;
		default:
			printk(KERN_INFO "Неопознанная команда\n");
			ret = -1;
			break;
	}
	return ret;
}

static ssize_t phonebook_write(struct file *file, const char __user *buf, size_t size, loff_t *offset) {
	if (size > MAX_COMMAND_SIZE) {
		printk(KERN_INFO "Команда слишком длинная\n");
		return -EINVAL;
	}
	memset(command, '\0', MAX_COMMAND_SIZE);
	if (copy_from_user(command, buf, size)) {
		printk(KERN_INFO "Плохой адрес (write)\n");
		return -EFAULT;
	}
	int ret = execute_command();
	if (ret) {
	
	}
	
	return size;
}

static int major;

static int __init phonebook_mod_init(void) {
	static struct file_operations fops = {
		.open = phonebook_open,
		.release = phonebook_release,
		.read = phonebook_read,
		.write = phonebook_write
	};
	major = register_chrdev(0, DEVICE_NAME, &fops);
	if (major < 0) {
		printk(KERN_INFO "Не удалось зарегистрировать устройство. Ошибка: %d\n", major);
		return major;
	}
	printk(KERN_INFO "Устройство зарегистрировано со старшим номером %d\n", major);
	printk(KERN_INFO "Команда 'mknod /dev/phonebook c %d 0' для создания файла устройства\n", major);
	printk(KERN_INFO "Попробуйте команды 'cat' и 'echo' к файлу устройства\n");
	printk(KERN_INFO "Удалите файл устройства и модуль по завершению\n");
	
	INIT_LIST_HEAD(&contacts);
	
	return 0;
}

static void __exit phonebook_mod_exit(void) {
	unregister_chrdev(major, DEVICE_NAME);
}

module_init(phonebook_mod_init);
module_exit(phonebook_mod_exit);



















