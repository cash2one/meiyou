#!/usr/bin/env python 
#coding: utf-8
import tensorflow as tf
import sys
import os
import os.path
import ctypes  
import time

# reload(sys)
# sys.setdefaultencoding('utf8')

#retrun value: -1 error; 0 not miji; 1 miji;
def identifyByVal(strFilePath):
	soPicVal = ctypes.cdll.LoadLibrary   
	try:
		libPicVal = soPicVal("./libPicVal.so") 
	except Exception as e:
		print "load lib failed"
		return -1

	nValNum = 64

	pic_val = "a" * nValNum
	try:
		bresult = libPicVal.getPicValString(strFilePath, pic_val, len(pic_val))  
		if bresult:
			# print 'hash : '
			# print pic_val
			1

		else:
			print pic_path, ": get value failed"
			return -1

	except:
		print pic_path, ": get pic value failed"
		return -1

	pic_val_file = open("./picval.txt", 'r')
	bmiji = 0
	for line in pic_val_file:
		lingstrip = line.strip()
		if len(lingstrip) > nValNum:
			pic_val_line = lingstrip[-nValNum:]
			ndifcount = 0
			# print line
			for i in range(nValNum):
				# print pic_val[i], pic_val_line[i]
				if pic_val[i] != pic_val_line[i]:
					ndifcount = ndifcount + 1
			if ndifcount <= 2:
				print lingstrip
				bmiji = 1
			# print "ndif", ndifcount


	pic_val_file.close()

	return bmiji

# bret = identifyByVal("./imglib/miji/6.jpg")
# print "是否密集：", bret

if len(sys.argv) != 2:
	print "please give and only give picture path or dir"
	sys.exit(0)

image_path = sys.argv[1]
#print(image_path)
if not(os.path.isfile(image_path) or os.path.isdir(image_path)):
	print "given path not a path or a directory"
	sys.exit(0)
 
labels = []
# for label in tf.gfile.GFile("output_labels.txt"):
# 	labels.append(label.rstrip())
labels.append("非密集恐惧")
labels.append("密集恐惧")


with tf.gfile.FastGFile("identifyData.pb", 'rb') as f:
	graph_def = tf.GraphDef()
	graph_def.ParseFromString(f.read())
	tf.import_graph_def(graph_def, name='')
 
# print "clock3:%s" % time.clock()  

with tf.Session() as sess:
	softmax_tensor = sess.graph.get_tensor_by_name('final_result:0')

	# print "clock4:%s" % time.clock() 

	if os.path.isfile(image_path):

		strResultFile = image_path + "_result.txt"
		fileResult = open(strResultFile, 'w')
		fileResult.write("{\n")
		fileResult.write("    \"items\": [\n")

		fResult = 0

		bjudgebyval = identifyByVal(image_path)

		if bjudgebyval == 1:
			print "by value"
			print labels[1]
			fResult = 1
		else:
			try:
				image = tf.gfile.FastGFile(image_path, 'rb').read()
				predict = sess.run(softmax_tensor, {'DecodeJpeg/contents:0': image})
				print image_path

				if predict[0][0] > 0.8:
					print labels[0]
				elif predict[0][1] > 0.95:
					print labels[1]
					fResult = 1
				else:
					# print "不确定"
					print labels[0]

				# top = predict[0].argsort()[-len(predict[0]):][::-1]
				# for index in top:
				# 	human_string = labels[index]
				# 	score = predict[0][index]
				# 	print human_string, score

			except:
				print "无效"

		fileResult.write("        {\n")
		fileResult.write("            \"image\": \"" + image_path + "\",\n")
		fileResult.write("            \"result\": " + str(fResult) + "\n")
		fileResult.write("        }\n")
		fileResult.write("    ],\n")
		fileResult.write("    \"status\": 1,\n")
		if fResult == 0:
			fileResult.write("    \"message\": \"非密集恐惧或无效图片\",\n")
		else:
			fileResult.write("    \"message\": \"密集恐惧图片\",\n")		
		fileResult.write("}\n")
		fileResult.close()


	elif os.path.isdir(image_path):

		strResultFile = image_path
		strResultFile = strResultFile.rstrip('\/ ') + "_result.txt"
		fileResult = open(strResultFile, 'w')
		fileResult.write("{\n")
		fileResult.write("    \"items\": [\n")

		nmiji = 0
		nbumiji = 0
		nbuqueding = 0
		nwuxiao = 0
		nid = 0
		for parent,dirnames,filenames in os.walk(image_path):    #三个参数：分别返回1.父目录 2.所有文件夹名字（不含路径） 3.所有文件名字
			for filename in filenames:                        #输出文件信息

				fResult = 0
				pic_path = ""
				try:

					# print "clock per start:%s" % time.clock() 

					pic_path = os.path.join(parent,filename)
					# print pic_path

					bjudgebyval = identifyByVal(pic_path)

					nid += 1
					print "id:", nid

					if bjudgebyval == 1:
						print "by value"
						print pic_path
						print labels[1], "+++++++++"
						nmiji += 1
						fResult = 1
					else:
						image = tf.gfile.FastGFile(pic_path, 'rb').read()
						# print "clock per middle:%s" % time.clock() 
						predict = sess.run(softmax_tensor, {'DecodeJpeg/contents:0': image})

						# print "no: ", predict[0][0], " ; yes: ", predict[0][1]

						if predict[0][0] > 0.8:
							# print pic_path
							# print labels[0], "---------"
							nbumiji += 1
						elif predict[0][1] > 0.95:
							print pic_path
							print labels[1], "+++++++++"
							nmiji += 1
							fResult = 1
						else:
							# print pic_path
							# print "不确定", "========="
							nbuqueding += 1

						# top = predict[0].argsort()[-len(predict[0]):][::-1]
						# for index in top:
						# 	human_string = labels[index]
						# 	score = predict[0][index]
						# 	print human_string, score

						# print "clock per end:%s" % time.clock() 

				except:
					# print pic_path
					# print "无效", "!!!!!!!!!"
					nwuxiao += 1

				fileResult.write("        {\n")
				fileResult.write("            \"image\": \"" + pic_path + "\",\n")
				fileResult.write("            \"result\": " + str(fResult) + "\n")
				fileResult.write("        },\n")				

		ntotal = nmiji + nbumiji + nbuqueding + nwuxiao
		print "总数 :", ntotal
		print "密集恐惧 :", nmiji, "  ", float(nmiji)/ntotal
		# print "不密集恐惧 :", nbumiji, "  ", float(nbumiji)/ntotal
		# print "不确定 :", nbuqueding, "  ", float(nbuqueding)/ntotal
		# print "无效 :", nwuxiao, "  ", float(nwuxiao)/ntotal

		fileResult.write("    ],\n")
		fileResult.write("    \"status\": 1,\n")
		fileResult.write("    \"message\": \"总数：" + str(ntotal) + "，其中密集图片：" + str(nmiji) + "\",\n")	
		fileResult.write("}\n")
		fileResult.close()

	else:
		print "warning: invalid path"

	# ftime = time.clock()/3 
	# print "clock5:%s" % ftime



