"""
V2.0
货币兑换程序
人民币与美元相互转化
2018/7/2
"""
rmb_num = 0
USD_CNY = 6.6541
while 1:
    str = input("请输入金额和种类:")
	num = eval(str[:-2])
	if num < 0:
        break
	print(num)
    if str[-3:] == 'CNY':	#人民币
		usd_num = num * USD_CNY
		print("美元金额:",usd_num)
	elif str[-3:] == 'USD':	#美元
		cny_num = num / USD_CNY
		print('人民币金额:',cny_num)
	else
		print("无法识别的货币种类")
    
print("输入金额错误,程序退出")
