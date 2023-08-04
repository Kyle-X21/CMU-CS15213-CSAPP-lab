# Bomb Lab
### 总结
本实验的主要目的就是让学生熟悉x86-64汇编语言，并掌握gdb调试器的使用。

### 实验要求
运行程序的时候，用户需要输入6个不同的字符串。若任何一个输入的字符串不正确，则炸弹爆炸。  
学生需要通过反汇编和逆向工程的方法，写出6个正确的字符串。  
需要理解汇编语言，需要学习使用debugger（主要是gdb调试器）。  

### 尝试
这个实验给的文件很少。能读的代码只有bomb.c。  
bomb.c中可以得到的有效信息是：  
1.可以把解出的字符串放到文本文件里，用命令行传参的方式传给main。当然在标准输入里输入也可以。
2.调用了一个函数initialize_bomb();可推测是初始化炸弹的函数。函数实现不可见。  
3.程序一行行读入输入，判断是否正确。\

现在根据官网提供的Writeup，执行一些gdb命令。
在ubuntu里输入指令objdump -d bomb > file.txt，把bomb可执行文件反汇编。放到Word里，以便于使用查找功能。\
可以直接查找“main”。找到了main函数的汇编代码。\
main里有一行
  > 400e19:	e8 84 05 00 00       	callq  4013a2 <initialize_bomb>

显然是对应.c文件里第67行
  > initialize_bomb();

### 第一个字符串
继续向下看汇编代码，main里有一行
 > 400e3a:	e8 a1 00 00 00       	callq  400ee0 <phase_1>

对应.c文件第74行
> phase_1(input);                  /* Run the phase               */

接下来看phase_1：
```C++
0000000000400ee0 <phase_1>:
  400ee0:	48 83 ec 08          	sub    $0x8,%rsp //栈指针减去8，即扩展8字节的栈空间
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi //0x402400 in esi
  400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal> //调用函数
  400eee:	85 c0                	test   %eax,%eax
  400ef0:	74 05                	je     400ef7 <phase_1+0x17>//和上一条指令结合，就是eax里是0就跳转，否则不跳转；
  400ef2:	e8 43 05 00 00       	callq  40143a <explode_bomb>
  400ef7:	48 83 c4 08          	add    $0x8,%rsp//这是跳转地址，也就是说，如果不跳转，炸弹就爆炸
  400efb:	c3                   	retq   
```
调用了一个函数<strings_not_equal>，大致看了下这个函数和main里的上下文，显然是比较0x402400地址上的字符串和输入的字符串。\
使用gdb查看0x402400地址上的字符串，是：
Border relations with Canada have never been better.\
这就是第一个字符串。测试可以通过。

### 第二个字符串
接下来看phase_2：
```C++
0000000000400efc <phase_2>:
  400efc:	55                   	push   %rbp
  400efd:	53                   	push   %rbx
  400efe:	48 83 ec 28          	sub    $0x28,%rsp//扩展40字节的栈空间
  400f02:	48 89 e6             	mov    %rsp,%rsi//栈指针作为第二个参数
  400f05:	e8 52 05 00 00       	callq  40145c <read_six_numbers>//大概看了下汇编代码，大概是读取6个数的意思；如果输入少于6个数，炸弹会爆炸。
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)
  400f0e:	74 20                	je     400f30 <phase_2+0x34>//add[rsp]必须和1相等，不然爆炸
  400f10:	e8 25 05 00 00       	callq  40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax//第一次：add[rbx-4] in eax，而rbx是rsp+4，所以eax里就是地址rsp上的值//第二次：eax=add[rsp+4]
  400f1a:	01 c0                	add    %eax,%eax//eax*=2
  400f1c:	39 03                	cmp    %eax,(%rbx)
  400f1e:	74 05                	je     400f25 <phase_2+0x29>//第一次：此时eax必须和add[rsp+4]的值相等，否则爆炸；结合上述，也就是，add[rsp]的2被必须等于add[rsp+4]，而add[rsp]为1，所以add[rsp+4]为2//第二次：此时eax必须==add[rsp+8]，所以add[rsp+8]=4//已经找到规律了，add[rsp+12]=8,add[rsp+16]=16,add[rsp+20]=32
  400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
  400f25:	48 83 c3 04          	add    $0x4,%rbx//rbx=rsp+8
  400f29:	48 39 eb             	cmp    %rbp,%rbx//第一次：rbp=tsp+24
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>
  400f2e:	eb 0c                	jmp    400f3c <phase_2+0x40>
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx//rsp+4 in rbx
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp//rsp+24 in rbp
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>
  400f3c:	48 83 c4 28          	add    $0x28,%rsp
  400f40:	5b                   	pop    %rbx
  400f41:	5d                   	pop    %rbp
  400f42:	c3                   	retq   

```
根据注释分析，要输入6个数，且这6个数分别是，1 2 4 8 16 32 \
经测试，第二个问题的答案确实就是这个。

### 第三个字符串
接下来看phase_3：
```C++ 
0000000000400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp//24字节栈空间
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx//rcx=rsp+12
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx//rdx=rsp+8
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi//esi=0x4025cf
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax//eax=0
  400f5b:	e8 90 fc ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  400f60:	83 f8 01             	cmp    $0x1,%eax
  400f63:	7f 05                	jg     400f6a <phase_3+0x27>//eax必须大于1，不然爆炸
  400f65:	e8 d0 04 00 00       	callq  40143a <explode_bomb>
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)//利用gdb，知道此时rsp=0x7fffffffdd60,并且add[rsp+8]就是输入的第一个参数
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a>//add[rsp+8]必须小于等于7，不然爆炸
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax//eax=add[rsp+8]
  400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)//跳转目标是add[402470+8*rax]；利用gdb，知道此时rax的值就是输入的第一个参数；利用gdb可以知道地址0x402470附近的数据，rax为0时，数据是0x400f7c
  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax//根据我们的解法，程序会跳转到这里
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>
  400fad:	e8 88 04 00 00       	callq  40143a <explode_bomb>
  400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
  400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>//eax必须等于add[rsp+12]，不然爆炸;利用gdb，知道add[rsp+12]就是输入的第二个数；根据我们的解法，第二个参数取0xcf，即207
  400fc4:	e8 71 04 00 00       	callq  40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	retq   

```
根据注释分析，得到一组正确的解是 0 207.
### 第四个字符串
接下来看phase_4：
```C++ 
000000000040100c <phase_4>:
  40100c:	48 83 ec 18          	sub    $0x18,%rsp//24字节栈空间
  401010:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx//rcx=rsp+12
  401015:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx//rdx=rsp+8
  40101a:	be cf 25 40 00       	mov    $0x4025cf,%esi//esi=0x4025cf
  40101f:	b8 00 00 00 00       	mov    $0x0,%eax
  401024:	e8 c7 fb ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  401029:	83 f8 02             	cmp    $0x2,%eax
  40102c:	75 07                	jne    401035 <phase_4+0x29>//输入的参数必须是2个，否则爆炸
  40102e:	83 7c 24 08 0e       	cmpl   $0xe,0x8(%rsp)
  401033:	76 05                	jbe    40103a <phase_4+0x2e>//add[rsp+8]必须<=14,否则爆炸
  401035:	e8 00 04 00 00       	callq  40143a <explode_bomb>
  40103a:	ba 0e 00 00 00       	mov    $0xe,%edx//edx=14
  40103f:	be 00 00 00 00       	mov    $0x0,%esi//esi=0
  401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi//edi=add[rsp+8]
  401048:	e8 81 ff ff ff       	callq  400fce <func4>
  40104d:	85 c0                	test   %eax,%eax
  40104f:	75 07                	jne    401058 <phase_4+0x4c>//eax必须是0，否则爆炸
  401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp)
  401056:	74 05                	je     40105d <phase_4+0x51>//add[rsp+12]必须是0，否则爆炸。即第二个参数为0.
  401058:	e8 dd 03 00 00       	callq  40143a <explode_bomb>
  40105d:	48 83 c4 18          	add    $0x18,%rsp
  401061:	c3                   	retq   

```
看一下phase_4里调用的func4的汇编代码：
```C++ 
0000000000400fce <func4>:
  400fce:	48 83 ec 08          	sub    $0x8,%rsp//8字节栈空间
  400fd2:	89 d0                	mov    %edx,%eax//eax=14
  400fd4:	29 f0                	sub    %esi,%eax//eax=14-0=14
  400fd6:	89 c1                	mov    %eax,%ecx//ecx=14
  400fd8:	c1 e9 1f             	shr    $0x1f,%ecx//0xe>>31，ecx=0
  400fdb:	01 c8                	add    %ecx,%eax//eax=14+0=14
  400fdd:	d1 f8                	sar    %eax//此时为默认移动1位，eax=7
  400fdf:	8d 0c 30             	lea    (%rax,%rsi,1),%ecx//ecx=7+0=7
  400fe2:	39 f9                	cmp    %edi,%ecx//比较7和add[rsp+8]
  400fe4:	7e 0c                	jle    400ff2 <func4+0x24>//如果7<=add[rsp+8],跳转，
  400fe6:	8d 51 ff             	lea    -0x1(%rcx),%edx
  400fe9:	e8 e0 ff ff ff       	callq  400fce <func4>
  400fee:	01 c0                	add    %eax,%eax
  400ff0:	eb 15                	jmp    401007 <func4+0x39>
  400ff2:	b8 00 00 00 00       	mov    $0x0,%eax//eax=0
  400ff7:	39 f9                	cmp    %edi,%ecx//比较14和add[rsp+8]
  400ff9:	7d 0c                	jge    401007 <func4+0x39>//如果14>=add[rsp+8]，跳转，退出了这个函数。所以add[rsp+8]只要为7就可以安全度过这个函数。即第一个参数为7.
  400ffb:	8d 71 01             	lea    0x1(%rcx),%esi
  400ffe:	e8 cb ff ff ff       	callq  400fce <func4>
  401003:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax
  401007:	48 83 c4 08          	add    $0x8,%rsp
  40100b:	c3                   	retq   

```
分析后可得一组正确的解是7 0.
### 第五个字符串
接下来看phase_5：
```C++ 
0000000000401062 <phase_5>:
  401062:	53                   	push   %rbx
  401063:	48 83 ec 20          	sub    $0x20,%rsp//32字节栈空间
  401067:	48 89 fb             	mov    %rdi,%rbx//rbx=rdi;rdi值是读入的字符串的地址
  40106a:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
  401071:	00 00 
  401073:	48 89 44 24 18       	mov    %rax,0x18(%rsp)//add[rsp+18]被赋值
  401078:	31 c0                	xor    %eax,%eax
  40107a:	e8 9c 02 00 00       	callq  40131b <string_length>
  40107f:	83 f8 06             	cmp    $0x6,%eax//输入字符串长度必须是6
  401082:	74 4e                	je     4010d2 <phase_5+0x70>
  401084:	e8 b1 03 00 00       	callq  40143a <explode_bomb>
  401089:	eb 47                	jmp    4010d2 <phase_5+0x70>
  40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx//第一次：ecx=add[rbx+0]，第一个字符;第二次：ecx=add[rbx+1]，第二个字符;依次类推，对6个字符都处理一次；
  40108f:	88 0c 24             	mov    %cl,(%rsp)//add[rsp]=cl=上一行赋的值
  401092:	48 8b 14 24          	mov    (%rsp),%rdx//rdx=add[rsp]=上一行赋的值
  401096:	83 e2 0f             	and    $0xf,%edx//保留edx最低4位
  401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx//edx=add[rdx+0x4024b0];用gdb调试发现edx的值取决于它自己的第四位加上这个地址；
  4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)//add[rsp+0+16]=dl=上一行赋的值
  //所以就是会把add[rsp+16~rsp+21]这六字节变成从内存中另外一个地方取出的值。
  4010a4:	48 83 c0 01          	add    $0x1,%rax//rax=1
  4010a8:	48 83 f8 06          	cmp    $0x6,%rax
  4010ac:	75 dd                	jne    40108b <phase_5+0x29>
  4010ae:	c6 44 24 16 00       	movb   $0x0,0x16(%rsp)//add[rsp+22]=0
  4010b3:	be 5e 24 40 00       	mov    $0x40245e,%esi//esi=0x40245e
  4010b8:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi//rdi=rsp+16
  4010bd:	e8 76 02 00 00       	callq  401338 <strings_not_equal>//比较地址0x40245e上的字符串和rsp+16上的字符串;用gdb发现这个值是"flyers"
  4010c2:	85 c0                	test   %eax,%eax
  4010c4:	74 13                	je     4010d9 <phase_5+0x77>//比较必须相同
  4010c6:	e8 6f 03 00 00       	callq  40143a <explode_bomb>
  4010cb:	0f 1f 44 00 00       	nopl   0x0(%rax,%rax,1)
  4010d0:	eb 07                	jmp    4010d9 <phase_5+0x77>
  4010d2:	b8 00 00 00 00       	mov    $0x0,%eax//eax=0
  4010d7:	eb b2                	jmp    40108b <phase_5+0x29>
  4010d9:	48 8b 44 24 18       	mov    0x18(%rsp),%rax//rax=add[rsp+24]
  4010de:	64 48 33 04 25 28 00 	xor    %fs:0x28,%rax
  4010e5:	00 00 
  4010e7:	74 05                	je     4010ee <phase_5+0x8c>//虽然不懂上一行什么意思，但知道肯定希望这一行执行跳转，从而结束phase_5.试验以后发现不用管这两行。根据之前得到的低4位构造出字符串即可。
  4010e9:	e8 42 fa ff ff       	callq  400b30 <__stack_chk_fail@plt>
  4010ee:	48 83 c4 20          	add    $0x20,%rsp
  4010f2:	5b                   	pop    %rbx
  4010f3:	c3                   	retq   

```
0x4024b0上是这么一个字符串："maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
需要的是flyers，f是+9，l是+15，y是+14，e是+5，r是+6，s是+7.\
所以构造出要输入的六个字符的低4位分别是1001 1111 1110 0101 0110 0111.\
所以一组解是：)/.567\
### 第六个字符串
接下来看phase_6：
```C++
00000000004010f4 <phase_6>:
  4010f4:	41 56                	push   %r14
  4010f6:	41 55                	push   %r13
  4010f8:	41 54                	push   %r12
  4010fa:	55                   	push   %rbp
  4010fb:	53                   	push   %rbx
  4010fc:	48 83 ec 50          	sub    $0x50,%rsp//80字节栈空间
  401100:	49 89 e5             	mov    %rsp,%r13//r13=rsp
  401103:	48 89 e6             	mov    %rsp,%rsi//rsi=rsp
  401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>//读取6个数
  40110b:	49 89 e6             	mov    %rsp,%r14//r14=rsp
  40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d//r12d=0


  401114:	4c 89 ed             	mov    %r13,%rbp//rbp=r13
  401117:	41 8b 45 00          	mov    0x0(%r13),%eax//eax=add[r13]=add[rsp+4*i]
  40111b:	83 e8 01             	sub    $0x1,%eax//eax=add[r13]-1
  40111e:	83 f8 05             	cmp    $0x5,%eax
  401121:	76 05                	jbe    401128 <phase_6+0x34>//eax必须小于等于5；这其实就是输入的数减去1,；所以输入的6个数都必须小于等于6.另外，很重要但容易忽略的一点是，be是无符号比较，所以这六个数减去1后肯定是非负数，所以这六个数就是1~6.
  401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>
  401128:	41 83 c4 01          	add    $0x1,%r12d//r12d=r12d+1=1;第二次：r12d=2
  40112c:	41 83 fc 06          	cmp    $0x6,%r12d
  401130:	74 21                	je     401153 <phase_6+0x5f>//等于6就跳转
  401132:	44 89 e3             	mov    %r12d,%ebx//ebx=r12d=1；第二次ebx=2
  401135:	48 63 c3             	movslq %ebx,%rax//rax=r12d=1；第二次就是2
  401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax//eax=add[rsp+ebx*4]
  40113b:	39 45 00             	cmp    %eax,0x0(%rbp)//add[rbp]（现在就是add[rsp]）和add[rsp+ebx*4]；循环会把add[rsp]和add[rsp+1*4~rsp+5*4]都比较一遍，也就是说add[rsp]必须和他后面五个东西不相等;第二次是add[rsp+4]和add[rsp+2*4~5*4]比较；这是个二重循环，总之就是确保了add[rsp]到add[rsp+5*4]这六个地址上的数互不相同。所以输入的6个数必须互不相同。
  40113e:	75 05                	jne    401145 <phase_6+0x51>//必须不相等
  401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>
  401145:	83 c3 01             	add    $0x1,%ebx//ebx=ebx+1=2
  401148:	83 fb 05             	cmp    $0x5,%ebx
  40114b:	7e e8                	jle    401135 <phase_6+0x41>//ebx<=5就跳转；最后为ebx被加到6
  40114d:	49 83 c5 04          	add    $0x4,%r13//r13=r13+4=rsp+4
  401151:	eb c1                	jmp    401114 <phase_6+0x20>


  401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi//rsi=rsp+24
  401158:	4c 89 f0             	mov    %r14,%rax//rax=r14=rsp
  40115b:	b9 07 00 00 00       	mov    $0x7,%ecx//ecx=7
  401160:	89 ca                	mov    %ecx,%edx//edx=ecx=7
  401162:	2b 10                	sub    (%rax),%edx//edx=edx-add[rax]=7-add[rsp]；第二次rax是rsp+4
  401164:	89 10                	mov    %edx,(%rax)//add[rax]=edx,即add[rsp]=7-add[rsp]
  401166:	48 83 c0 04          	add    $0x4,%rax//rax=rsp+4
  40116a:	48 39 f0             	cmp    %rsi,%rax//比较rax和rsp+24
  40116d:	75 f1                	jne    401160 <phase_6+0x6c>//不相等就跳转，这又是一个循环；可以看出这个循环执行的任务就是，把add[rsp~rsp+20]这6个地址上的数都变成7减去自己。


  40116f:	be 00 00 00 00       	mov    $0x0,%esi//esi=0
  401174:	eb 21                	jmp    401197 <phase_6+0xa3>

  401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx//rdx=add[rdx+8]，此时edx=6032d0.那猜测这个链表，rdx+8位置是next指针。用gdb查看，0x6032d8上的4字节数据，是0x6032e0.显然是下一个节点的地址。这确实是个next指针。
  40117a:	83 c0 01             	add    $0x1,%eax//eax=2；eax每次+1
  40117d:	39 c8                	cmp    %ecx,%eax
  40117f:	75 f5                	jne    401176 <phase_6+0x82>//不相等就往回跳，rdx一直在读以自己为基址的一个值，这有点像链表；rax一直+1，直到和ecx相等；//这个ecx现在就是第一个输入的大于1的数。最后rax被加成这个数。那其实就是读到指定编号的结点，结点的地址在rdx中。
  401181:	eb 05                	jmp    401188 <phase_6+0x94>//等到rax=ecx了，跳转



  401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx//edx=0x6032d0

  401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2)//add[rsp+2*rsi+32]=rdx，rsi的值取range(0,24,4)。//这边其实是把指定结点的地址传给了栈。所以比如输入的数据是3 2 1 6 5 4。那程序就会先把第三个结点放到栈里，然后把第2个结点放到栈里。在栈中的地址是从rsp+32开始按顺序+8递增的。所以会用到rsp+72
  //从0x401181跳过来，此时rsi=0
  40118d:	48 83 c6 04          	add    $0x4,%rsi//rsi=rsi+4,第一次是4，每次+4
  401191:	48 83 fe 18          	cmp    $0x18,%rsi
  401195:	74 14                	je     4011ab <phase_6+0xb7>//rsi如果等于24，跳转

  401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx//ecx=add[rsp+rsi]=rsi取0,4,8，一直到20
  40119a:	83 f9 01             	cmp    $0x1,%ecx
  40119d:	7e e4                	jle    401183 <phase_6+0x8f>//如果ecx<=1，跳转
  //上面这个也是个循环，离开这个循环有2种情况，一种是ecx>1，就是说某个add[rsp+rsi]值>1了，那就离开循环；第二种情况是rsi已经被加到了24，也会离开循环。但是离开循环的目的地址不同。

//记住，add[rsp]以后的6*4个字节，就是输入的6个数。根据前面分析，这六个数就是1~6.第一次进这个循环，这个循环执行的任务就是找到了大于1的一个输入数，并且根据其在输入中的序号给栈中对应位置add[rsp+32+2*(序号-1)*4]赋地址为0x6032d0.这个地址用gdb查看值，显示node，显然这个程序用到了一个链表数据结构。
//通过情况一离开循环后，一定会再回到循环，所以最终一定是通过情况二离开。就是会遍历一遍输入的所有数。

//综合上述分析，上面这一大块的循环，就是按照输入的数据，把对应编号的结点的地址放到了栈里。

//估计链表头节点地址就是0x6032d0，查看这个地址，上面的32位数据是0x014c。
  40119f:	b8 01 00 00 00       	mov    $0x1,%eax//通过情况1离开循环，会来到这里。eax=1
  4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx//edx=0x6032d0
  4011a9:	eb cb                	jmp    401176 <phase_6+0x82>
  4011ab:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx//通过情况2离开循环，来到这里。rbx=add[rsp+32].根据上述分析，这就是在栈中的头节点地址。
  4011b0:	48 8d 44 24 28       	lea    0x28(%rsp),%rax//rax=rsp+40//根据上述分析，这是栈中下一个结点的地址。
  4011b5:	48 8d 74 24 50       	lea    0x50(%rsp),%rsi//rsi=rsp+80//栈顶地址。
  4011ba:	48 89 d9             	mov    %rbx,%rcx//rcx=rbx=add[rsp+32]

  4011bd:	48 8b 10             	mov    (%rax),%rdx//rdx=add[rax];rax从rsp+40开始，一直做到rsp+72.这就是把栈里对应结点的地址给了rdx。
  4011c0:	48 89 51 08          	mov    %rdx,0x8(%rcx)//add[rcx+8]=rdx。rcx本身是栈里对应结点的地址，一开始是头节点地址。那add[rcx+8]就是头节点的next指针的值，现在把头节点的下一个结点改成了栈里的第二个结点的地址。
  4011c4:	48 83 c0 08          	add    $0x8,%rax//rax=rsp+48。rax不断+8，就是不断到栈里放下一个结点地址的地方。
  4011c8:	48 39 f0             	cmp    %rsi,%rax//这一段也是一个循环，要一直进行到rax到达rsp+80
  4011cb:	74 05                	je     4011d2 <phase_6+0xde>
  4011cd:	48 89 d1             	mov    %rdx,%rcx//rax还没到rsp+80,rcx=rdx=add[上一个rax]；所以rcx其实是从add[rsp+32]开始，一直做到add[rsp+72]。rcx就变成了原先结点的地址。比如说，刚到这里的时候就是栈里第二个结点的地址。
  4011d0:	eb eb                	jmp    4011bd <phase_6+0xc9>//分析到这里就明白了，上面这个循环的作用，就是把本来的6个结点，按照栈里的顺序重新组织了起来。这个顺序取决于输入的数据。比如输入 3 2 1 5 6 4.那现在链表的顺序就是3号结点，2号结点，5号结点，这样。

  4011d2:	48 c7 42 08 00 00 00 	movq   $0x0,0x8(%rdx)//现在rax=rsp+80.add[rdx+8]=0.rdx应该是add[rsp+72]//这显然是给新链表附上了末尾的null结点。
  4011d9:	00 
  4011da:	bd 05 00 00 00       	mov    $0x5,%ebp//ebp=5


  4011df:	48 8b 43 08          	mov    0x8(%rbx),%rax//rax=add[rbx+8]，rbx=add[rsp+32]。所以rax就是头节点的下一个结点的地址。
  4011e3:	8b 00                	mov    (%rax),%eax//eax=add[rax]。获取第二个结点的值。
  4011e5:	39 03                	cmp    %eax,(%rbx)//比较add[rbx]和rax，rbx应该是add[rsp+32].就是比较add[add[rsp+32]]和add[add[rbx+8]]。就是比较第一个节点的值和第二个结点的值。
  4011e7:	7d 05                	jge    4011ee <phase_6+0xfa>//上个比较必须是大于等于
  4011e9:	e8 4c 02 00 00       	callq  40143a <explode_bomb>
  4011ee:	48 8b 5b 08          	mov    0x8(%rbx),%rbx//rbx=add[rbx+8]。rbx等于下一个结点的地址。
  4011f2:	83 ed 01             	sub    $0x1,%ebp//ebp-=1
  4011f5:	75 e8                	jne    4011df <phase_6+0xeb>//ebp-1后为0就结束了//上面这个循环会做五次，就是链表的六个结点，重组之后必须是降序。
//至此已经分析完了。查看这六个结点上的数据值是多少就行。头节点地址是0x6032d0,+8后获取下一个结点的地址。注意上面用于比较的是eax，这是32位寄存器，所以查看的是地址上的32位的数据。
//头节点是0x14c，2号结点是0xa8,3号结点是0x39c,4号是0x2b3,5号是0x1dd，6号是0x1bb。大小顺序按降序是，3号，4号，5号，6号，1号，2号，这是我们期望的顺序。但是注意，程序用7减去过输入的数据。所以恢复成原数据是4,3,2,1,6,5.

  4011f7:	48 83 c4 50          	add    $0x50,%rsp
  4011fb:	5b                   	pop    %rbx
  4011fc:	5d                   	pop    %rbp
  4011fd:	41 5c                	pop    %r12
  4011ff:	41 5d                	pop    %r13
  401201:	41 5e                	pop    %r14
  401203:	c3                   	retq   

```
这个汇编程序很复杂，有好几个循环，并且使用了链表这一数据结构。因此需要耐心来阅读分析代码。\
据以上分析，解是 4 3 2 1 6 5.
终于通过了所有phase。

### 隐藏phase
bomb.c中phase_6之后有一个注释：
```C++
    /* Wow, they got it!  But isn't something... missing?  Perhaps
     * something they overlooked?  Mua ha ha ha ha! */
```
说明还有一个隐藏的Phase.在汇编代码的phase_6后面也能看到一个secret_phase。查找phase_defused，看到在phase_defused中调用了phase_defused。既然叫secret，说明肯定是符合条件才能进入这个phase。下面分析。
```C++
//这是phase_defused中的一部分
 4015f0:	be 19 26 40 00       	mov    $0x402619,%esi//用gdb查看以后知道是需要读取两个整数，一个字符串
  4015f5:	bf 70 38 60 00       	mov    $0x603870,%edi//scanf的第二个参数，指定了从哪里读取。
  4015fa:	e8 f1 f5 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  4015ff:	83 f8 03             	cmp    $0x3,%eax//要输入三个数据
  401602:	75 31                	jne    401635 <phase_defused+0x71>
  401604:	be 22 26 40 00       	mov    $0x402622,%esi//上面值是DrEvil
  401609:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi//rdi=rsp+16，这应该是输入的字符串地址。用gdb查看以后会发现这个地址和phase4中输入字符串的地址是一样的。所以其实就是在phase_4中再额外输入一个字符串进入这个隐藏phase。
  40160e:	e8 25 fd ff ff       	callq  401338 <strings_not_equal>//说明要输入的就是DrEvil.
  401613:	85 c0                	test   %eax,%eax
  401615:	75 1e                	jne    401635 <phase_defused+0x71>
  401617:	bf f8 24 40 00       	mov    $0x4024f8,%edi//edi=0x4024f8
  40161c:	e8 ef f4 ff ff       	callq  400b10 <puts@plt>
  401621:	bf 20 25 40 00       	mov    $0x402520,%edi
  401626:	e8 e5 f4 ff ff       	callq  400b10 <puts@plt>
  40162b:	b8 00 00 00 00       	mov    $0x0,%eax//eax=0
  401630:	e8 0d fc ff ff       	callq  401242 <secret_phase>
  401635:	bf 58 25 40 00       	mov    $0x402558,%edi
  40163a:	e8 d1 f4 ff ff       	callq  400b10 <puts@plt>
  40163f:	48 8b 44 24 68       	mov    0x68(%rsp),%rax
  401644:	64 48 33 04 25 28 00 	xor    %fs:0x28,%rax
  40164b:	00 00 
  40164d:	74 05                	je     401654 <phase_defused+0x90>
  40164f:	e8 dc f4 ff ff       	callq  400b30 <__stack_chk_fail@plt>
  401654:	48 83 c4 78          	add    $0x78,%rsp
  401658:	c3                   	retq   
```
看secret_phase和fun7的汇编：
```C++

0000000000401242 <secret_phase>:
  401242:	53                   	push   %rbx
  401243:	e8 56 02 00 00       	callq  40149e <read_line>
  401248:	ba 0a 00 00 00       	mov    $0xa,%edx//edx=10
  40124d:	be 00 00 00 00       	mov    $0x0,%esi//esi=0
  401252:	48 89 c7             	mov    %rax,%rdi//rdi=rax，这应该是输入的字符串的地址
  401255:	e8 76 f9 ff ff       	callq  400bd0 <strtol@plt>//把输入的字符串转换成整数
  40125a:	48 89 c3             	mov    %rax,%rbx//rbx=rax
  40125d:	8d 40 ff             	lea    -0x1(%rax),%eax//eax=rax-1
  401260:	3d e8 03 00 00       	cmp    $0x3e8,%eax//输入的数-1必须小于等于0x3e8，就是十进制1000
  401265:	76 05                	jbe    40126c <secret_phase+0x2a>
  401267:	e8 ce 01 00 00       	callq  40143a <explode_bomb>
  40126c:	89 de                	mov    %ebx,%esi//esi=ebx，应该是转换的整数，这是第二个参数
  40126e:	bf f0 30 60 00       	mov    $0x6030f0,%edi//edi=0x6030f0，这是第一个参数
  401273:	e8 8c ff ff ff       	callq  401204 <fun7>
  401278:	83 f8 02             	cmp    $0x2,%eax//把返回值和2比较
  40127b:	74 05                	je     401282 <secret_phase+0x40>//必须返回2
  40127d:	e8 b8 01 00 00       	callq  40143a <explode_bomb>
  401282:	bf 38 24 40 00       	mov    $0x402438,%edi
  401287:	e8 84 f8 ff ff       	callq  400b10 <puts@plt>
  40128c:	e8 33 03 00 00       	callq  4015c4 <phase_defused>
  401291:	5b                   	pop    %rbx
  401292:	c3                   	retq   

0000000000401204 <fun7>:
  401204:	48 83 ec 08          	sub    $0x8,%rsp
  401208:	48 85 ff             	test   %rdi,%rdi//rdi是第一个参数，初始是0x6030f0，上面值是0x24
  40120b:	74 2b                	je     401238 <fun7+0x34>//为0就跳转
  40120d:	8b 17                	mov    (%rdi),%edx//edx=add[rdi]；说明第一个参数应该是个地址（指针）
  40120f:	39 f2                	cmp    %esi,%edx//比较地址上的值和esi，esi是第二个参数
  401211:	7e 0d                	jle    401220 <fun7+0x1c>//小于等于就跳转
  401213:	48 8b 7f 08          	mov    0x8(%rdi),%rdi//没跳转，说明大于
  401217:	e8 e8 ff ff ff       	callq  401204 <fun7>//递归调用，第一个参数由上一行更新为add[rdi+8]
  40121c:	01 c0                	add    %eax,%eax//这说明如果是大于，返回2fun7(add[rdi+8],esi)
  40121e:	eb 1d                	jmp    40123d <fun7+0x39>//调用结束了。
  401220:	b8 00 00 00 00       	mov    $0x0,%eax//eax=0
  401225:	39 f2                	cmp    %esi,%edx//比较了地址上的值和第二个参数
  401227:	74 14                	je     40123d <fun7+0x39>//相等就跳转，这就离开本次调用了。说明相等就返回0.
  401229:	48 8b 7f 10          	mov    0x10(%rdi),%rdi//不相等的话，说明是小于，rdi=add[rdi+16]。寄存器以自己为基址来更新自己，这又是很像链表的结构。
  40122d:	e8 d2 ff ff ff       	callq  401204 <fun7>//递归调用
  401232:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax//eax=2rax+1，就是等于上一次调用的返回值*2+1，说明如果是小于，就返回2fun7(add[rdi+16],esi)+1。第一个参数在递归调用时更新，第二个参数不更新。
  401236:	eb 05                	jmp    40123d <fun7+0x39>//调用结束了。
  401238:	b8 ff ff ff ff       	mov    $0xffffffff,%eax//eax=-1
  40123d:	48 83 c4 08          	add    $0x8,%rsp
  401241:	c3                   	retq   
//结合上述分析，fun7可以根据一个大小关系，往两个指针的方向继续调用自己。这其实就是一棵树的结构。
//分析完下面的secret_phase后，知道希望返回2.那就构造这样的递归：第一次调用，返回2fun7(add[rdi+8],esi),第二次调用，返回2fun7(add[rdi+16],esi)+1，第三次调用，返回0.那么希望第一次比较结果是大于，第二次比较结果是小于，第三次比较结果是等于。用gdb查看这些地址上的值，，第一次调用，结点值是0x24，第二次调用上面值是0x8，第三次调用0x16，那正好就是输入0x16,即10进制的22.这样正好满足我们构造的递归。试验也确实可以。

```
我们给出一个解是22.
至此，所有phase就都完成了。
