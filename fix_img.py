"""Fix malformed img tag: remove base64 garbage after display:block; in homePage.html"""
import re

with open(r"c:\Projects\Brennan\Backend\homePage.html", "r", encoding="utf-8") as f:
    content = f.read()

# Find: Polish_20240811_215410422.jpeg img with display:block;" followed by base64 until "/>
start_marker = 'display:block;"'
end_marker = '"/>'
idx = content.find('Polish_20240811_215410422')
if idx >= 0:
    chunk = content[idx:]
    start = chunk.find(start_marker)
    if start >= 0:
        search_from = idx + start + len(start_marker)
        end = content.find(end_marker, search_from)
        if end >= 0:
            remove_start = search_from
            remove_end = end + len(end_marker)
            new_content = content[:remove_start] + '/>' + content[remove_end:]
            with open(r"c:\Projects\Brennan\Backend\homePage.html", "w", encoding="utf-8") as f:
                f.write(new_content)
            print("Fix applied successfully.")
        else:
            print("Could not find closing />")
    else:
        print("Could not find start marker")
else:
    print("Could not find image")
