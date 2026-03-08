with open('homePage.html', 'r', encoding='utf-8') as f:
    content = f.read()

old = '''    <div class="photo-card">
       <div class="photo-placeholder team-ph" style="padding:0;background:none;">
      </div>'''

new = '''    <div class="photo-card">
       <div class="photo-placeholder team-ph" style="padding:0;background:none;">
         <img src="Polish_20240811_215410422.jpeg" alt="Team member" style="width:100%;height:100%;object-fit:cover;display:block;">
      </div>'''

if old in content:
    content = content.replace(old, new, 1)
    with open('homePage.html', 'w', encoding='utf-8') as f:
        f.write(content)
    with open('result.txt', 'w') as r:
        r.write('OK')
else:
    with open('result.txt', 'w') as r:
        r.write('NOT FOUND')
