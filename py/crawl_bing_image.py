#-*- coding:utf-8 -*-
import os
import sys
import re
import requests

def get_url(keywords, image_index):
    word = " ".join(keywords)
    return f"https://cn.bing.com/images/async?q={word}&first={image_index}&count=35&relp=35&lostate=r&mmasync=1"

def find_image_urls(url):
    image_urls = []
    headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/73.0.3683.86 Safari/537.36'}
    r = requests.get(url, headers=headers)
    if r.status_code == 200:
        # <img class="mimg" style="background-color:#91553a;color:#91553a" height="191" width="297"
        # src="https://tse4-mm.cn.bing.net/th/id/OIP.686kUSgKfcC72jB5KVwE-wHaEw?w=297&amp;h=191&amp;c=7&amp;o=5&amp;pid=1.7" alt="打电话 的图像结果">
        image_urls = re.findall(r'<img\s+class="mimg"\s+style=".+?"\s+height=".+?"\s+width=".+?"\s+src="(.+?)"\s+alt=".+?">', r.text, re.S)
    return image_urls

def download(url):
    data = None
    try:
        r = requests.get(url)
        data = r.content if r.status_code == 200 else None
    except BaseException as e:
        print(f"Downloading error: {e} ")
    return data

def main(keywords, out_dir, num_images):
    image_index = 0
    anchor_index = 0
    while image_index < num_images:
        image_urls = find_image_urls(get_url(keywords, anchor_index))
        anchor_index += 35
        for i, url in enumerate(image_urls):
            print(f"Downloading image %06d from %s" % (image_index , url))
            data = download(url)
            if data:
                suffix = url[url.rfind(".")+1:]
                if suffix.lower() in ["jpg", "png"]:
                    out_file = "%06d.%s" % (image_index, suffix)
                else:
                    out_file = "%06d.jpg" % image_index
                out_path = os.path.join(out_dir, out_file)
                with open(out_path, "wb") as f:
                    f.write(data)
                    image_index += 1
                    if image_index >= num_images:
                        return

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <out-image-dir> <out-image-nums> <keyword1> [keyword2 [...]] ")
        sys.exit(-1)
    out_dir = sys.argv[1]
    num_images = int(sys.argv[2])
    keywords = sys.argv[3:]
    if not os.path.isdir(out_dir):
        os.mkdir(out_dir)
    main(keywords, out_dir, num_images)
