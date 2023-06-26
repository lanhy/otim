//
//  ViewController.m
//  TestSDK
//
//  Created by lanhy on 2017/8/31.
//  Copyright © 2017年 lanhy. All rights reserved.
//

#import "ViewController.h"
#import <UIKit/UIKit.h>
#import <OMTPChatAPI.h>
#include "otim.h"
//#include "otim_err.h"
#include <iostream>

#define NSS_TO_STDS(a)  (a.length > 0 ? a.UTF8String:"")

#define STDS_TO_NSS(a)  (a.length() > 0 ?  [NSString stringWithCString:a.c_str() encoding:NSUTF8StringEncoding]:nil)

#define CHARS_TO_NSS(a)  ((a != NULL && strlen(a) > 0) > 0 ?  [NSString stringWithCString:a encoding:NSUTF8StringEncoding]:nil)


#define TC_LOGINRESP    @"TC_LOGINRESP"
#define TC_NETSTATUS    @"TC_NETSTATUS"
#define TC_KICKOUT      @"TC_KICKOUT"
#define TC_HOTSESSION   @"TC_HOTSESSION"
#define TC_MSGRECV      @"TC_MSGRECV"
#define TC_MSGACK       @"TC_MSGACK"
#define TC_MSGRECV      @"TC_MSGRECV"


class CTNImsdkCallbackImpl : public otim::IOtimSdkCallback {
    virtual void netStatusChanged(int32_t status){
        NSLog(@"IMPL netStatusChanged:%d", status);
       
        [[NSNotificationCenter defaultCenter] postNotificationName:TC_NETSTATUS object:nil userInfo:@{@"status":@(status)}];
    }

    virtual void loginResp(int32_t code, const std::string & clientId){
        NSLog(@"IMPL loginResp code:%d",code);
        
        [[NSNotificationCenter defaultCenter] postNotificationName:TC_LOGINRESP object:nil userInfo:@{@"clientId":STDS_TO_NSS(clientId),@"code":@(code)}];
    }
    virtual void kickOut(){
        NSLog(@"IMPL kickOut");
        [[NSNotificationCenter defaultCenter] postNotificationName:TC_KICKOUT object:nil userInfo:@{@"code":@(5)}];
    }
    
    /*
     * 热会话响应
     */
    virtual void hotSessionResp(otim::HotSessionResp* hotSessions){
        NSLog(@"IMPL hotSessionResp");
        std::string json = hotSessions->writeToJsonString();
        NSString * nsJson = STDS_TO_NSS(json);
        NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:[nsJson dataUsingEncoding:NSUTF8StringEncoding] options:kNilOptions error:nil];

        [[NSNotificationCenter defaultCenter] postNotificationName:TC_HOTSESSION object:nil userInfo:dict];

    }
  
    
//    /**
//     * 收到在线消息
//     * @param req 消息内容
//     */
    virtual void msgRecv(int type, const std::string &packId, otim::MsgReq* req){
        NSLog(@"IMPL msgRecv:%d %s, %s", type, packId.c_str(), req->sessionId.c_str());
        std::string json = req->writeToJsonString();
        NSString * nsJson = STDS_TO_NSS(json);
        NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:[nsJson dataUsingEncoding:NSUTF8StringEncoding] options:kNilOptions error:nil];

        [[NSNotificationCenter defaultCenter] postNotificationName:TC_MSGRECV object:nil userInfo:dict];
   }

    /**
     * 发送消息响应
     * @param sessionId 会话Id
     * @param msgId msgId
     * @param seqId seqId
     * @param result 0 成功，非0 失败；
     */
    virtual void msgAck(const std::string& packId, otim::MsgAck* ack){
        NSLog(@"IMPL msgAck:%s %ld ret:%d", packId.c_str(), ack->seqId, ack->errorCode.code);
        std::string json = ack->writeToJsonString();
        NSString * nsJson = STDS_TO_NSS(json);
        NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:[nsJson dataUsingEncoding:NSUTF8StringEncoding] options:kNilOptions error:nil];
        [[NSNotificationCenter defaultCenter] postNotificationName:TC_MSGACK object:nil userInfo:dict];
    }
 
    /**
     * 用户在线状态变化通知
     * @param mapClientStatus 对应表 {用户名 在线状态} 0 不在线，1在线
     */
    virtual void userOnlineStatus(std::map<string, int32_t> &mapClientStatus){
        NSLog(@"IMPL userOnlineStatus:%d", mapClientStatus.size());

//        [[NSNotificationCenter defaultCenter] postNotificationName:TNC_KICKOUT object:@{@"code":@(5)}];
   }
    
private:
    std::string _clientId;
};



@interface ViewController ()
@property(strong)UITextView* textview;
@property(strong)UITextView* textviewSend;
@property (nonatomic) otim::IOtimSdk *imsdk;
@property (nonatomic) CTNImsdkCallbackImpl *callback;
@property (nonatomic, strong) NSString *username;
@property (nonatomic, strong) NSString *password;
@property (nonatomic, strong) UITextField *userIdTF;
@property (nonatomic, strong) UITextField *userToTF;

@end

@implementation ViewController

void testVector(CGRect &rect){
    rect.origin.x = 100;
    rect.origin.y = 200;
    
}
- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Do any additional setup after loading the view, typically from a nib.
    self.userIdTF = [UITextField new];
    self.userIdTF.frame = CGRectMake(80,100, 200, 30);
    [self.userIdTF setBorderStyle:UITextBorderStyleLine];
    [self.userIdTF setPlaceholder:@"ClientId"];
    
    [self.view addSubview:self.userIdTF];
    
    self.userToTF = [UITextField new];
    self.userToTF.frame = CGRectMake(80,150, 200, 30);
    [self.userToTF setBorderStyle:UITextBorderStyleLine];
    
    [self.userToTF setPlaceholder:@"To"];
    
    [self.view addSubview:self.userToTF];
    
    UIButton* btnLogin= [UIButton buttonWithType:UIButtonTypeSystem];
    btnLogin.frame = CGRectMake(100, 200, 60, 30);
    btnLogin.backgroundColor = [UIColor brownColor];
    [btnLogin setTitle:@"Login" forState:UIControlStateNormal];
    [btnLogin setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [btnLogin addTarget:self action:@selector(loginClick) forControlEvents:UIControlEventTouchUpInside];
    
    [self.view addSubview:btnLogin];
    
    
    UIButton* btnSend = [UIButton buttonWithType:UIButtonTypeSystem];
    btnSend.frame = CGRectMake(180,200, 60, 30);
    btnSend.backgroundColor = [UIColor brownColor];
    [btnSend setTitle:@"Send" forState:UIControlStateNormal];
    [btnSend setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [btnSend addTarget:self action:@selector(sendClick) forControlEvents:UIControlEventTouchUpInside];
    
    [self.view addSubview:btnSend];
    
    
    
    self.textviewSend = [UITextView new];
    self.textviewSend.frame = CGRectMake(0, 320, self.view.bounds.size.width, 60);
    self.textviewSend.backgroundColor = [UIColor blackColor];
    self.textviewSend.textColor =[UIColor whiteColor];
    [self.view addSubview:self.textviewSend];
    self.textviewSend.text = @"测试消息内容";
    
    
    self.textview = [UITextView new];
    self.textview.frame = CGRectMake(0, 400, self.view.bounds.size.width, 400);
    self.textview.backgroundColor = [UIColor lightGrayColor];
    [self.view addSubview:self.textview];
    
    
    self.username = @"";
    self.password = @"9_xmAo5INAjbbEjctZ9JVw7jGJ82i60-afqqqtcpBRM=";
    
    [[NSNotificationCenter defaultCenter] addObserverForName:TC_NETSTATUS object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        NSData *data = [NSJSONSerialization dataWithJSONObject:note.userInfo options:kNilOptions error:nil];// OC对象 -> JSON数据       NSLog(@"%@", data);
        dispatch_sync(dispatch_get_main_queue(), ^{
            self.textview.text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        });
    }];
    
    [[NSNotificationCenter defaultCenter] addObserverForName:TC_LOGINRESP object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        NSData *data = [NSJSONSerialization dataWithJSONObject:note.userInfo options:kNilOptions error:nil];// OC对象 -> JSON数据       NSLog(@"%@", data);
        dispatch_sync(dispatch_get_main_queue(), ^{
            self.textview.text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        });
    }];
    [[NSNotificationCenter defaultCenter] addObserverForName:TC_MSGACK object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        NSData *data = [NSJSONSerialization dataWithJSONObject:note.userInfo options:kNilOptions error:nil];// OC对象 -> JSON数据       NSLog(@"%@", data);
        dispatch_sync(dispatch_get_main_queue(), ^{
            self.textview.text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        });
    }];
    [[NSNotificationCenter defaultCenter] addObserverForName:TC_MSGRECV object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        NSData *data = [NSJSONSerialization dataWithJSONObject:note.userInfo options:kNilOptions error:nil];// OC对象 -> JSON数据       NSLog(@"%@", data);
        dispatch_sync(dispatch_get_main_queue(), ^{
            self.textview.text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        });
    }];
    [[NSNotificationCenter defaultCenter] addObserverForName:TC_HOTSESSION object:nil queue:nil usingBlock:^(NSNotification * _Nonnull note) {
        NSData *data = [NSJSONSerialization dataWithJSONObject:note.userInfo options:kNilOptions error:nil];// OC对象 -> JSON数据       NSLog(@"%@", data);
        dispatch_sync(dispatch_get_main_queue(), ^{
            self.textview.text = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        });
    }];
    
}

- (NSString *)uuidString
{
    CFUUIDRef uuid_ref = CFUUIDCreate(NULL);
    CFStringRef uuid_string_ref= CFUUIDCreateString(NULL, uuid_ref);
    NSString *uuid = [NSString stringWithString:(__bridge NSString *)uuid_string_ref];
    CFRelease(uuid_ref);
    CFRelease(uuid_string_ref);
    
    return [uuid lowercaseString];
}

-(void) initSdk{
    NSString* clientId = self.userIdTF.text;
    TNClientInfo clientInfo;
    clientInfo.clientId = NSS_TO_STDS(clientId);
    NSString* uuid = [self uuidString];
    clientInfo.deviceId = NSS_TO_STDS(uuid);
//    clientInfo.userType = 31;
//    clientInfo.authType = 1;
    clientInfo.username = NSS_TO_STDS(self.username);
    clientInfo.password = NSS_TO_STDS(self.password);

    clientInfo.deviceType = 1;//iOS
    clientInfo.appType = 2;
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSLibraryDirectory, NSUserDomainMask, YES);
    NSString *docDir = [paths objectAtIndex:0];

    clientInfo.appPath = NSS_TO_STDS(docDir);
    
    NSString* version = [[NSBundle mainBundle] infoDictionary][@"CFBundleShortVersionString"];
    clientInfo.version = NSS_TO_STDS(version);
    
   otim::initLog(clientInfo.appPath.c_str());
    
    self.imsdk = otim::initIm(clientInfo);
//    self.imsdk->addHostInfo("127.0.0.1", 10012, false);
//   self.imsdk->addHostInfo("imv5.qa.huohua.cn", 80, false);
//    self.imsdk->addHostInfo("10.250.200.136", 10012, false);
//    self.imsdk->addHostInfo("39.101.1.141", 12008, false);
        self.imsdk->addHostInfo("10.250.0.112", 19000, false);


    if (self.callback == nil){
        self.callback = new CTNImsdkCallbackImpl();
    }
    
    self.imsdk->setCallback(self.callback);

}

-(void) loginClick{
    [self initSdk];
    NSLog(@"loginClick");
    self.imsdk->login(NSS_TO_STDS(self.username), NSS_TO_STDS(self.password));
}

- (void)didReceiveMessageWithNotification:(NSNotification *)notification
{
//    NSDictionary * dic = notification.userInfo;
//    TNCMessage * message = (TNCMessage *)dic[@"message"];
//    dispatch_async(dispatch_get_main_queue(), ^{
//        self.textview.text = message.content;
//    });
//    //    dispatch_ansync
//    //    [self didReceiveMessageNotification:message];
}

-(void) sendMessage:(NSString *) from to:(NSString *) to text:(NSString*) text{
    if (to.length == 0){
        self.textview.text = @"接受者ID未填写";
        return;;
    }
    
    NSString *sessionId;
    if ([from compare:to] >= 0){
        sessionId = [NSString stringWithFormat:@"SC_%@_%@", to, from];

    }
    else{
        sessionId = [NSString stringWithFormat:@"SC_%@_%@", from, to];
    }
   
    otim::MsgReq msgReq;
    msgReq.sessionId = NSS_TO_STDS(sessionId);
    msgReq.from = NSS_TO_STDS(from);
    msgReq.to = NSS_TO_STDS(to);
    msgReq.content = NSS_TO_STDS(text);
    self.imsdk->sendMessage(otim::PT_MSG_SINGLE_CHAT, msgReq);
}

-(void) sendClick{
    NSLog(@"sendClick");
    if (self.imsdk == nullptr){
        self.textview.text = @"未登录";
        return;;
    }
    
    
    [self sendMessage:self.userIdTF.text to:self.userToTF.text text:self.textviewSend.text];
    
    
//    for (int i = 0; i < 2; i++){
//        NSString* clientId = [NSString stringWithFormat:@"305%03d", i];
//
//    }
    
}

- (void)testDBClick{
//    [TNIM_CHATLIB testDB];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


@end
