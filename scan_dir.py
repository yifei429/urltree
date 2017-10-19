#!/usr/bin/env python  
#coding=utf-8  
  
import os  
import sys
  
def scan_files(directory,prefix=None,postfix=None):  
    files_list=[]  
      
    for root, sub_dirs, files in os.walk(directory):  
        for special_file in files:  
            if postfix:  
                if special_file.endswith(postfix):  
                    files_list.append(os.path.join(root,special_file))  
            elif prefix:  
                if special_file.startswith(prefix):  
                    files_list.append(os.path.join(root,special_file))  
            else:  
                files_list.append(os.path.join(root,special_file))  
                            
    return files_list  


#scan a website, such as phpwind, ecshop, opensns
if __name__ == "__main__":
	if len(sys.argv) != 3:
		print("Usage: python ./test <scandir> <out.txt>")
		exit()
	website = sys.argv[2]
	rootdir = sys.argv[1]
	print("scan dir %s, result in file:%s" % (rootdir, website));
	files_list = scan_files(rootdir)
	#print(files_list);
	if os.path.exists(website):
		os.remove(website)
	with open(website, 'w') as f:
		for files in files_list: 
			f.write(files[len(rootdir) -1 :])
			f.write('\n')
	
