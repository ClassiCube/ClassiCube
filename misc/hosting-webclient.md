So you want to host your own version of the webclient? It's pretty easy to do, you need 3 files:
1) The game .js file
2) A web page to initialise the .js and display the game
3) The default texture pack

### Example setup

For example, let's assume our site is setup like this:
* `example.com/play.html`
* `example.com/static/classisphere.js`
* `example.com/static/default.zip`

For simplicitly,
1) Download `classicube.net/static/classicube.js`, then upload it to `static/classisphere.js` on the webserver
2) Download `classicube.net/static/default.zip`,   then upload it to `static/default.zip` on the webserver

The play.html page is the trickiest part, because how to implement it is website-specific. (depends on how your website is structured, what webserver you use, etc)

### Example play.html

TODO
