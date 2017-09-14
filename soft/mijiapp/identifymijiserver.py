# coding: utf-8
__author__ = 'linjinbin'
import tensorflow as tf
import sys
import ctypes
import urllib2
import logging
import os
from flask import Flask, request, jsonify
import time

reload(sys)
sys.setdefaultencoding('utf8')

logging.basicConfig(level=logging.WARNING,
                    format='%(asctime)s %(filename)s[line:%(lineno)d] %(levelname)s %(message)s',
                    datefmt='%a, %d %b %Y %H:%M:%S',
                    filename='log/myapp.log',
                    filemode='w')

labels = [u"非密集恐惧", u"密集恐惧", u"无法识别"]
headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:23.0) Gecko/20100101 Firefox/23.0'}

with tf.gfile.FastGFile("identifyData.pb", 'rb') as f:
    logging.info(u"开始加载pb文件...")
    start = time.clock()
    graph_def = tf.GraphDef()
    graph_def.ParseFromString(f.read())
    tf.import_graph_def(graph_def, name='')
    end = time.clock()
    logging.info(u"加载pb文件结束!耗时:"+str(end-start))

# 使用Flask框架实现文本落地识别
app = Flask(__name__)

libPicVal = None
picValLines = []


def loadLib():
    soPicVal = ctypes.cdll.LoadLibrary
    try:
        logging.info(u"开始加载lib文件...")
        start = time.clock()
        libPicVal = soPicVal("./libPicVal.so")
        end = time.clock()
        logging.info(u"lib文件加载完成!耗时:"+str(end-start))
        return libPicVal
    except Exception as e:
        logging.error(u"lib文件加载失败!" )
        print e
        return None


def loadPicValLines():
    try:
        logging.info(u"开始加载picval文件...")
        start = time.clock()
        pic_val_file = open("./picval.txt", 'r')
        for line in pic_val_file:
            picValLines.append(line)
        pic_val_file.close()
        end = time.clock()
        logging.info(u"picval文件加载完成!耗时:"+str(end-start))
    except Exception as e:
        logging.error(u"加载picval文件失败!")
        print e


# retrun value: -1 error; 0 not miji; 1 miji;
def identifyByVal(strFilePath):
    nValNum = 64
    pic_val = "a" * nValNum
    try:
        bresult = libPicVal.getPicValString(strFilePath, pic_val, len(pic_val))
        if bresult:
            1
        else:
            return -1
    except:
        return -1
    bmiji = 0
    for line in picValLines:
        lingstrip = line.strip()
        if len(lingstrip) > nValNum:
            pic_val_line = lingstrip[-nValNum:]
            ndifcount = 0
            for i in range(nValNum):
                if pic_val[i] != pic_val_line[i]:
                    ndifcount = ndifcount + 1
            if ndifcount <= 2:
                print lingstrip
                bmiji = 1
    return bmiji


def downloadImage(image_path, url):
    logging.info(u"开始下载图片:"+url)
    start = time.clock()
    request = urllib2.Request(url, None, headers)
    response = urllib2.urlopen(request)
    f = open(image_path, 'wb')
    f.write(response.read())
    f.close()
    end = time.clock()
    logging.info(u"图片下载完成!耗时:"+str(end-start))

@app.route("/identify", methods=['POST', 'GET'])
def index():
    return "Hello World!"

@app.route("/identify/miji", methods=['POST', 'GET'])
def identify():
    start = time.clock()
    result = {}
    if not request.args.get('url'):
        result["code"] = -1
        result["message"] = u"输入参数错误!"
    else:
        try:
            url = request.args.get('url')
            logging.info(u"开始识别图片:" + url)
            image_path = "tmp/" + url.split("/")[-1]
            downloadImage(image_path, url)
            start2 = time.clock()
            bjudgebyval = identifyByVal(image_path)
            if bjudgebyval == 1:
                result["code"] = 1
                result["message"] = labels[1]
            else:
                try:
                    image = tf.gfile.FastGFile(image_path, 'rb').read()
                    predict = sess.run(softmax_tensor, {'DecodeJpeg/contents:0': image})
                    print image_path
                    if predict[0][0] > 0.8:
                        result["code"] = 0
                        result["message"] = labels[0]
                    elif predict[0][1] > 0.95:
                        result["code"] = 1
                        result["message"] = labels[1]
                    else:
                        result["code"] = 2
                        result["message"] = labels[2]
                except  Exception as e:
                    logging.error(u"识别密集恐怖图片失败!" )
                    print e
                    result["code"] = 2
                    result["message"] = labels[2]
        except  Exception as e:
            logging.error(u"识别密集恐怖图片失败!")
            print e
            result["code"] = 2
            result["message"] = labels[2]
        if os.path.exists(image_path):
            os.remove(image_path)
    end = time.clock()
    logging.info(u"识别图片结束!耗时:"+str(end-start)+u" 其中计算耗时:"+str(end-start2))
    return jsonify(result)


if __name__ == '__main__':
    libPicVal = loadLib()
    loadPicValLines()
    sess = tf.Session()
    softmax_tensor = sess.graph.get_tensor_by_name('final_result:0')
    logging.info(u"资源文件加载完成!")
    if libPicVal == None or len(picValLines) == 0:
        logging.error(u"资源文件加载失败!")
    else:
        app.run(debug=True)
        # url = "http://mycdn.seeyouyima.com/news/img/148d70e3f71a2cfb826157bee1895e12_600_399.jpg";
        # print url + "结果：" + str(identify(url))
        # url = "http://mycdn.seeyouyima.com/news/img/03b0d72eb388b3087b114242c3b53cd9_582_573.jpg";
        # print url + "结果：" + str(identify(url))
        # url = "https://gss0.baidu.com/-4o3dSag_xI4khGko9WTAnF6hhy/zhidao/pic/item/91529822720e0cf393038d6c0c46f21fbf09aab5.jpg"
        # print url + "结果：" + str(identify(url))
