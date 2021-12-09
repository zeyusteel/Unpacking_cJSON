# Unpacking_cJSON
## 开箱cJSON源码
(https://github.com/DaveGamble/cJSON)  
(https://sourceforge.net/projects/cjson/)  
cJSON整体业务逻辑不多,理解源码并不难，这里只提炼一些当时通过阅读源码学到的知识点,cJSON代码很精炼，比较值得学习
## 核心结构体
所有的json结构都会复用这一个结构体，不同json类型通过type字段(对象、数组、ture、false、数字、字符串、NULL）来区分

```C
/* The cJSON structure: */
typedef struct cJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct cJSON *next;
    struct cJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct cJSON *child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==cJSON_String  and type == cJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} cJSON;
```
cJSON针对内存分配和释放有两个钩子,通过`cJSON_InitHooks(cJSON_Hooks* hooks);` 去设置

```C
typedef struct cJSON_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJSON_Hooks;
```

## 使用
cJSON的使用方法在cJSON源码工程中的test.c中

## 核心入口
`extern cJSON *cJSON_Parse(const char *value);`  
传入json字符串返回一个cJSON结构体，这个结构体需要通过 `cJSON_Delete`去释放。

## 核心解析函数
`static const char *parse_value(cJSON *item,const char *value)`会通过对传入的字符串进行解析，判断7种基本的json type,不断构建item.因为json的格式中`value`可以嵌套多种数据类型，所以解析到value时这个`prase_value`也有可能被递归调用。

这个时早期版本的解析函数, 如果解析成功的话，返回值应该指向`value`的末尾
```c
static const char *parse_value(cJSON *item,const char *value)
{
	if (!value)						return 0;	/* Fail on null. */
	if (!strncmp(value,"null",4))	{ item->type=cJSON_NULL;  return value+4; }
	if (!strncmp(value,"false",5))	{ item->type=cJSON_False; return value+5; }
	if (!strncmp(value,"true",4))	{ item->type=cJSON_True; item->valueint=1;	return value+4; }
	if (*value=='\"')				{ return parse_string(item,value); }
	if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
	if (*value=='[')				{ return parse_array(item,value); }
	if (*value=='{')				{ return parse_object(item,value); }

	ep=value;return 0;	/* failure. */
}
```
1.1.7版本的解析,代码的可读性更高了，解析的字符串也从字符指针变成了结构体，通过偏移量记录位置
```c
/* Parser core - when encountering text, process appropriately. */
static cJSON_bool parse_value(cJSON * const item, parse_buffer * const input_buffer)
{
    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return false; /* no input */
    }

    /* parse the different types of values */
    /* null */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "null", 4) == 0))
    {
        item->type = cJSON_NULL;
        input_buffer->offset += 4;
        return true;
    }
    /* false */
    if (can_read(input_buffer, 5) && (strncmp((const char*)buffer_at_offset(input_buffer), "false", 5) == 0))
    {
        item->type = cJSON_False;
        input_buffer->offset += 5;
        return true;
    }
    /* true */
    if (can_read(input_buffer, 4) && (strncmp((const char*)buffer_at_offset(input_buffer), "true", 4) == 0))
    {
        item->type = cJSON_True;
        item->valueint = 1;
        input_buffer->offset += 4;
        return true;
    }
    /* string */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '\"'))
    {
        return parse_string(item, input_buffer);
    }
    /* number */
    if (can_access_at_index(input_buffer, 0) && ((buffer_at_offset(input_buffer)[0] == '-') || ((buffer_at_offset(input_buffer)[0] >= '0') && (buffer_at_offset(input_buffer)[0] <= '9'))))
    {
        return parse_number(item, input_buffer);
    }
    /* array */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '['))
    {
        return parse_array(item, input_buffer);
    }
    /* object */
    if (can_access_at_index(input_buffer, 0) && (buffer_at_offset(input_buffer)[0] == '{'))
    {
        return parse_object(item, input_buffer);
    }

    return false;
}
```
## 随记  
1. 一个类型占一位， 方便位操作
```c
#define cJSON_Invalid (0)
#define cJSON_False  (1 << 0) //1
#define cJSON_True   (1 << 1) //2
#define cJSON_NULL   (1 << 2) //4
#define cJSON_Number (1 << 3) //8
#define cJSON_String (1 << 4) //16
#define cJSON_Array  (1 << 5) //32
#define cJSON_Object (1 << 6) //64
#define cJSON_Raw    (1 << 7) //128   /* raw json */

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512
```

2. 取消以前的true false 宏定义
```c
// .h
typedef int cJSON_bool; 
// .c
#ifdef true 
#undef true
#endif
#define true ((cJSON_bool)1)

#ifdef false
#undef false
#endif
#define false ((cJSON_bool)0)
```

3. 默认显示当前文件的符号的可见性.设置为default时，没有显式标识为hidden的符号都处理为可见
```c
#ifdef __GNUC__
#pragma GCC visibility push(default)
#endif
//这里的符号无论-fvisibility选项如何设置，都会输出
#ifdef __GNUC__
#pragma GCC visibility pop
#endif
```

4. `#if !defined`和`#ifndef`意思基本一致，但是`if #defined`可以用 && 连接多个条件`#if defined`同理 
```c
#if !defined(_CRT_SECURE_NO_DEPRECATE) && defined(_MSC_VER)
#define _CRT_SECURE_NO_DEPRECATE
#endif
```
5. 记录位置、大小多用`size_t`

6. 对于比较长的if条件可以封装成宏
```c
/* check if the given size is left to read in a given parse buffer (starting with 1) */
#define can_read(buffer, size) ((buffer != NULL) && (((buffer)->offset + size) <= (buffer)->length))
/* check if the buffer can be accessed at the given index (starting with 0) */
#define can_access_at_index(buffer, index) ((buffer != NULL) && (((buffer)->offset + index) < (buffer)->length))
#define cannot_access_at_index(buffer, index) (!can_access_at_index(buffer, index))
/* get a pointer to the buffer at the position */
#define buffer_at_offset(buffer) ((buffer)->content + (buffer)->offset)
```

7. 解析字符时间在新版使用`strtod`,旧版本需要将字符串逐字节分析
```c
static const char *parse_number(cJSON *item,const char *num)
{
	double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

	if (*num=='-') sign=-1,num++;	/* Has sign? */
	if (*num=='0') num++;			/* is zero */
	if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
	if (*num=='.' && num[1]>='0' && num[1]<='9') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* Fractional part? */
	if (*num=='e' || *num=='E')		/* Exponent? */
	{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* With sign? */
		while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* Number? */
	}

	n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* number = +/- number.fraction * 10^+/- exponent */
	
	item->valuedouble=n;
	item->valueint=(int)n;
	item->type=cJSON_Number;
	return num;
}
```
8. Unicode字符集,(\u00ff 这种表示)固定使用16bit(两个字节)表示一个字符。  
   对应三套编码方式（UTF-8、UTF-16、UTF-32）,他们都是Unicode的实现方式，其中UTF-16采用双字节编码，UTF-32采用四个字节。而UTF-8是变长的编码方式,可以使用(1~4)个字节,保留了ASCII  
    1）对于单字节的符号，字节的第一位设为0，后面7位为这个符号的`Unicode`码。因此对于英语字母，`UTF-8`编码和`ASCII`码是相同的。  
    2）对于`n`字节的符号`（n > 1）`，第一个字节的前`n`位都设为`1`，第`n + 1`位设为`0`，后面字节的前两位一律设为`10`。剩下的没有提及的二进制位，全部为这个符号的`Unicode`码。  
   
Unicode符号范围     |        UTF-8编码方式  
-------------------|-----------------------------
(十六进制)           |         （二进制) 
0000 0000 - 0000 007F | 0xxxxxxx
0000 0080 - 0000 07FF | 110xxxxx 10xxxxxx
0000 0800 - 0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
0001 0000 - 0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx

跟据上表，解读 UTF-8 编码非常简单。如果一个字节的第一位是0，则这个字节单独就是一个字符；如果第一位是1，则连续有多少个1，就表示当前字符占用多少个字节。
下面，还是以汉字严为例，演示如何实现 UTF-8 编码。
严的 Unicode 是4E25（100111000100101），根据上表，可以发现4E25处在第三行的范围内（0000 0800 - 0000 FFFF），因此严的 UTF-8 编码需要三个字节，即格式是1110xxxx 10xxxxxx 10xxxxxx。然后，从严的最后一个二进制位开始，依次从后向前填入格式中的x，多出的位补0。这样就得到了，严的 UTF-8 编码是11100100 10111000 10100101，转换成十六进制就是E4B8A5。

