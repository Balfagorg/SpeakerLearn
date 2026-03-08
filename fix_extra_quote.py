with open(r"c:\Projects\Brennan\Backend\homePage.html", "r", encoding="utf-8") as f:
    content = f.read()

# Fix the double quote: display:block;""/> -> display:block;"/>
content = content.replace('display:block;""/>', 'display:block;"/>')

with open(r"c:\Projects\Brennan\Backend\homePage.html", "w", encoding="utf-8") as f:
    f.write(content)
print("Fixed extra quote.")
