This repo was made to assist in finding and removing a deprecated topic from a company website. 

It is very similar to (Py-Sitemap)[https://github.com/jarydpeters/Py-Sitemap/tree/main], but while that repo is written in Python and was made to create a sitemap and detect 404 links, this project is written in C++ and crawls a website for mentions of a specific phrase.

Features:

-Detects input phrase witin a URL and its subpages  
-Outputs pages that contain detected phrase to "matched_urls.txt"  
-Avoids externally-linked websites

It does _not_ use SSL verification and is vulnerable to man-in-the-middle attacks if it were to be used in a production environment

Example output for finding mentions of a deprecated topic on a company's website:

![image](https://github.com/user-attachments/assets/d6050ce9-027c-46f3-a0ff-247cae280d31)

