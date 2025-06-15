New-Item -ItemType Directory -Path "D:\e-commerce"
New-Item -ItemType File -Path "D:\e-commerce\index.html" -Force
New-Item -ItemType Directory -Path "D:\e-commerce\css"
New-Item -ItemType File -Path "D:\e-commerce\css\style.css" -Force
New-Item -ItemType Directory -Path "D:\e-commerce\javascript"
New-Item -ItemType File -Path "D:\e-commerce\javascript\script.js" -Force
Write-Host "Basic e-commerce website folder structure created in D:\e-commerce."
Write-Host "You'll need to manually add HTML, CSS, and JavaScript code to the files to build the actual website."
Write-Host "Consider using a proper IDE like VS Code for developing the website."
