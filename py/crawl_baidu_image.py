#-*- coding:utf-8 -*-
import os
import sys
import re
import requests




def get_url(keywords, pn):
    word = " ".join(keywords)
    return f"http://image.baidu.com/search/flip?tn=baiduimage&ie=utf-8&word={word}&pn={pn}"

def find_image_urls(url):
    # image_urls = ["http://www.kaixian.tv/gd/d/file/201407/29/0a47ee95f37ff8174cc916150471fa1f.jpg"]
    image_urls = []
    r = requests.get(url)
    if r.status_code != 200:
        return image_urls
    # "objURL": "http://www.kaixian.tv/gd/d/file/201407/29/0a47ee95f37ff8174cc916150471fa1f.jpg"
    image_urls = re.findall(r'"objURL"\s*:\s*"(.*?)",', r.text, re.S)
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
    pn = 0
    while image_index < num_images:
        image_urls = find_image_urls(get_url(keywords, pn))
        pn += 60
        for url in image_urls:
            print(f"Downloading image %06d from %s" % (image_index, url))
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
