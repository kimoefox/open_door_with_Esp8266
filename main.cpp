//在第27和28行写入你的2.4G的wifi的SSID和密码
//在代码的第37，38，39行写入关于企业微信的数据     
//第49~52行写入Bark设置的数据  
//第191行写入你的打卡人的名称
//如果不需要bark服务请直接删除265~311行的代码和383行的代码

//包含的库
#include <ESP8266WiFi.h>           //WiFi模块
#include <ESP8266HTTPClient.h>     //网页请求模块
#include <WiFiClient.h>            //Https访问模块
#include <NTPClient.h>             //服务器获取时间模块
#include <WiFiUdp.h>               //UDP通讯模块
#include <ArduinoJson.h>           //Json解码模块
#include <Servo.h>                 //马达控制器

//设置token相关
long rctime = 0;      //设置rctime,记录正确获取了Token的时间
long svtime = 0;      //从NTP服务器里面获取的时间
int extime = 0;       //设置extime，计算时间差用
String mytoken,mytoken0;   //设置mytoken值
String Link;      //设置Link变量,就是url后面的路径

//debug
String qwqdoor = "plzopen";

//wifi 信息
const char* ssid = "SSID";        //把SSID替换成你的wifi名称
const char* password = "PSWD";    //把PSWD替换成你的wifi密码

//规定UDP链接
WiFiUDP ntpUDP;

// 企业微信相关设置
const char* host = "qyapi.weixin.qq.com";       // 设置主机名
const int httpsPort = 443;                      // 设置https端口
String url = "";                                // 请求的页面地址(后面代码中进行拼接)
const char fingerprint[] PROGMEM = "7A 40 AE 3E 56 98 D8 FF DF EE 5E 91 B3 53 0A 27 B4 C1 D9 5C";   //设置企业微信api的对应证书指纹  注意有时候需要更新证书指纹
String corp_id = "DDDD";                                                              // 把DDDD替换成你的企业微信的企业id
String corp_secret = "PPPP";                                 // 把你的PPPP替换成你的企业微信的打卡应用secret
String posturl;

//马达端口
Servo md001;                                    //设置存在一个叫做md的马达

//设置重启检测
int wrec = 0;                                      

//Bark
const char fingerprint2[] PROGMEM = "2a 43 86 cd 2c 6e 76 b5 b6 84 28 fd be 2e 27 96 bf d6 af 6f";   //设置BARKapi的对应证书指纹  注意有时候需要更新证书指纹
const char* barkhost = "api.day.app";       // 设置主机名
const int barkPort = 443;                      // 设置https端口
const char* barkLink = "";  //设置你专属的bark url

String resout4door;
int heap;



//开机设参
void setup(void)
{
  Serial.begin(9600);                 //设置“波特率”
  Serial.print("连接中...");
  WiFi.begin(ssid, password);           //链接wifi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("无线局域网已连接");

  Serial.println(WiFi.localIP());       //输出我的局域网ip

  NTPClient timeClient(ntpUDP, "cn.pool.ntp.org", 3600, 60000);     //启动时间客户端,设置服务器1,并连接
  timeClient.begin();                                                   //连接
  //刷新NTP
  timeClient.update();                                                  //刷新ntp,第一次
  Serial.println(timeClient.getFormattedTime());                        //显示ntp时间
  delay(5000);
  md001.attach(3, 500, 2500);                    //设置马达基础参数
  Serial.println("马达识别成功，正在归正");
  md001.write(0);                                //马达归零
  Serial.println("马达已归正");                   //设置初始值为0度
}


//设置重启函数
void(* resetFunc) (void) = 0;


//获取Token
String get_token(){
    //启动客户端
    WiFiClientSecure httpsClient;    //启动一个客户端 httpsClient
    Serial.println(host);            //显示域名
    Serial.printf("Using fingerprint '%s'\n", fingerprint);     //显示指纹
    httpsClient.setFingerprint(fingerprint);                    //设置指纹
    httpsClient.setTimeout(30000);                              //设置30S的超时时间
    delay(1000);                                                //暂停1秒
    //进行Https链接
    //设置等待链接动画
    Serial.print("HTTPS链接中");
    int r = 0; //retry counter
    while ((!httpsClient.connect(host, httpsPort)) && (r < 30))
    {
      delay(100); Serial.print("."); r++;
    }
    if (r == 30)
    {
      Serial.println("连接失败");
      wrec = wrec + 9;
      Serial.println("连接失败，喜加九，即将重启");

    }
    else
    {
      Serial.println("连接成功");
    }

    String getData, Link;


    //发送Get请求
    //设置url
    Link = "/cgi-bin/gettoken?corpid=" + corp_id + "&corpsecret=" + corp_secret;
    //发起链接
    httpsClient.print(String("GET ") + Link + " HTTP/1.1\r\n" +
                      "Host: " + host + "\r\n" +
                      "Connection: close\r\n\r\n");
    Serial.println("请求已发送");         //提示请求已发送

    //当连接成功时
    while (httpsClient.connected())
    {
      String line = httpsClient.readStringUntil('\n');
      if (line == "\r")
      {
        Serial.println("请求已收到");  //提示请求发送成功
        rctime = svtime;    //更新时间
        Serial.println("本次获取Token的时间是：  ");
        Serial.println(rctime);
        break;
      }
    }
    //打印结果
    //Serial.println("您的返回值是: ");
    //Serial.println("==========");
    String line;

  


    while (httpsClient.available())
    {
      line = httpsClient.readStringUntil('\n');   //设置逐行显示
      //Serial.println(line);                       //显示结果
    }
    //Serial.println("==========");
    //Serial.println("链接停止, 开始分析Token");
    //提取token
    //DynamicJdonDocument对象
    const size_t capacity = JSON_OBJECT_SIZE(2) + 384;   //设置内存大小
    DynamicJsonDocument doc(capacity);                   //将Json放入doc容器
    String json1 = line;                                 //开始解析
    deserializeJson(doc, json1);                         //反序列化
    mytoken = doc["access_token"].as<String>();          //提取信息


    //Serial.println("更新后的token是：");
    //Serial.println(mytoken);
    
    Serial.println("");
    Serial.println("");
    Serial.println("");
  return mytoken;
}


//检测打卡
String door_open(long svtime,String mytoken0){
  posturl = String("https://qyapi.weixin.qq.com/cgi-bin/checkin/getcheckindata?access_token=") + mytoken0;;
  //编写json
  Serial.println("正在编写json");
  long posttime_1 = svtime - 10;
  long posttime_2 = svtime + 10;
  String jsonPost = String("") + "{" +
                    "\"opencheckindatatype\": 3," +
                    "\"starttime\": " + posttime_1 + "," +
                    "\"endtime\": " + posttime_2 + "," +
                    "\"useridlist\": [\"zhangsan\",\"lisi\",\"wangwu\",\"liuliu\"]" +   //我们宿舍是4个人，你们可以以此类推
                    "}";
  Serial.print("json编写完毕");
  //启动客户端2
  Serial.println("正在启动https验证");
  WiFiClientSecure qywx2;    //Declare object of class WiFiClient
  qywx2.setFingerprint(fingerprint);
  qywx2.setTimeout(30000); // 15 Seconds
  delay(500);
  //进行Https链接
  Serial.println("开始连接");
  HTTPClient qywx3;
  qywx3.begin(qywx2, posturl);
  qywx3.addHeader("Content-Type", "Application/Json");
  int httpCode = qywx3.POST(jsonPost);
  String payload = qywx3.getString();
  Serial.println(httpCode);
  qywx3.end();
  Serial.println("链接完成");


  // 1：DynamicJsonDocument对象
  const size_t capacity = JSON_OBJECT_SIZE(2) + 1024;
  DynamicJsonDocument doc(capacity);
  // 2：反序列化数据
  deserializeJson(doc, payload);
  // 3：获取解析后的数据信息
  JsonObject checkindata_0 = doc["checkindata"][0];
  const char* gpn = checkindata_0["groupname"];
  String exception1 = checkindata_0["exception_type"];
  String block1 = "未打卡";
  int gid = checkindata_0["groupid"];
  long errcode = doc["errcode"];


  if (httpCode == -1) {
    wrec = wrec + 7;
    Serial.println("连接失败，喜加一");
  };
  if (errcode == 41001) {
    wrec = wrec + 5;
    Serial.println("没有Token，喜加一");
  };
  if (errcode == 40014) {
    wrec = wrec + 8;
    Serial.println("Token错误，喜加一");
  };
  if ( wrec > 18 ) {
    Serial.print("错误等级累计为");
    Serial.println(wrec);
    Serial.println("达到阈值，自动重启");
    resetFunc();
  };


  // 4:通过串口监视器输出解析后的数据信息
  Serial.print("gnStr = "); Serial.println(gpn);
  Serial.print("gid = "); Serial.println(gid);
  Serial.print("状态是 "); Serial.println(exception1);
if ( exception1 == block1)
  {
    Serial.println("阻止开锁成功");
    return("");
  }
  else
  {
    if ( gid == 1 ) {
      return "plzopen";
    }
    return("");
  }
}


//bark提示
String plzbark(String barkLink) {

  WiFiClientSecure bark;    //启动一个客户端 bark
  Serial.println(barkhost);            //显示域名
  Serial.printf("使用的HTTPS指纹是 '%s'\n", fingerprint2);     //显示指纹
  bark.setFingerprint(fingerprint2);                    //设置指纹
  bark.setTimeout(30000);                              //设置30S的超时时间
  delay(1000);                                                //暂停1秒
  //进行Https链接
    //设置等待链接动画
      Serial.print("HTTPS链接中");
      int r2=0; //retry counter
      while((!bark.connect(barkhost, barkPort)) && (r2 < 30))
      {
      delay(100);Serial.print(".");r2++;
      }
      if(r2==30) 
      {
      Serial.println("连接失败");
      }
      else 
      {
      Serial.println("连接成功");
    }

      String getData1;


  //发送Get请求
  //发起链接
  bark.print(String("GET ") + barkLink + " HTTP/1.1\r\n" +
      "Host: " + barkhost + "\r\n" +
      "Connection: close\r\n\r\n");
  Serial.println("请求已发送");         //提示请求已发送
      
  //当连接成功时   
  while (bark.connected())     
    {
      String line2 = bark.readStringUntil('\n');
      if (line2 == "\r") 
        {
          break;
        }
    }
    return("1");             
  }


//主函数
void loop() 
{ 
   //计算时间差
  while (svtime < 160000) {
    NTPClient timeClient(ntpUDP, "ntp.tencent.com");       //设置获取时间的服务器2
    timeClient.update();                                   //获取时间,第二次更新,别问为什么,就只能这样,程序依赖bug运行
    svtime = (timeClient.getEpochTime());             //记录服务器时间
    Serial.println(svtime);
    delay(1000);
    wrec = wrec + 1;
    if (wrec > 5) {
      Serial.println("无法获取时间，自动重启");
      resetFunc();
    }
  }
  NTPClient timeClient(ntpUDP, "ntp.tencent.com");       //设置获取时间的服务器3
  timeClient.update();                                   //获取时间,第三次更新,别问为什么,就只能这样,程序依赖bug运行
  svtime = (timeClient.getEpochTime());
  Serial.print("上次记录时间戳为 ");                      //输出时间比较
  Serial.println(rctime);
  Serial.print("当前的时间戳为 ");
  Serial.println(svtime);
  extime = svtime - rctime;
  while (svtime < 1649517391) {
    timeClient.update();
    svtime = (timeClient.getEpochTime());
    extime = svtime - rctime;
    wrec = wrec + 1;
    if (wrec > 5) {
      Serial.println("无法获取时间，自动重启");
      resetFunc();
    }
    Serial.println(svtime);
  }
  Serial.print("距离上次获取时间戳已经过了 ");
  Serial.print(extime);
  Serial.println("秒钟");

  //判断token获取时间是否大于1.9小时
  if (extime > 164160)     //是
    {
    mytoken0 = get_token();
    }


  Serial.println("现在暂时不需要更新Token了"); 

  //=======================================结束判断时间======================================

  resout4door = door_open(svtime,mytoken0) ;


  //舵机转动开门
  if ( qwqdoor == resout4door ) //有，转动3s舵机并循环
  {
    Serial.println("识别成功");
    int pos;
    Serial.println("正在开锁");
    for (pos = 0; pos <= 80; pos += 6) {
      md001.write(pos);
      delay(20);
    }
    Serial.println("即将关锁");
    for (pos = 80; pos >= 0; pos -= 6) {
      md001.write(pos);
      delay(15);
    }
    Serial.println("门锁已关闭");
    String openbark = plzbark(barkLink);
    delay(10000);
  }
  else //否，循环
    {
    Serial.println("等待按钮行为");
    };
  delay(1800);
}
//空