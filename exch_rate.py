"""
V1.0
货币兑换函数
人民币转美元
2018/7/2
"""
rmb_num = 0
USD_CNY = 6.6541
while 1:
    rmb_str = input("请输入人民币金额:")
    
    rmb_num = eval(rmb_str)
    if rmb_num < 0:
        break
    usd_num = rmb_num * USD_CNY
    
    print("美元金额:",usd_num)
print("输入金额错误,程序退出")
