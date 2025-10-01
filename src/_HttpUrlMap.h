/* #define URL_REMAP_FUNC(src_base, src_host, dst_base, dst_host) */

URL_REMAP_FUNC( "http://dl.dropbox.com/",             "dl.dropbox.com", 
				"https://dl.dropboxusercontent.com/", "dl.dropboxusercontent.com")

URL_REMAP_FUNC( "https://dl.dropbox.com/",            "dl.dropbox.com", 
				"https://dl.dropboxusercontent.com/", "dl.dropboxusercontent.com")

URL_REMAP_FUNC( "https://www.dropbox.com/",           "www.dropbox.com", 
				"https://dl.dropboxusercontent.com/", "dl.dropboxusercontent.com")

URL_REMAP_FUNC( "https://www.imgur.com/",             "www.imgur.com",
				"https://i.imgur.com/",               "i.imgur.com")

URL_REMAP_FUNC( "https://imgur.com/",                 "imgur.com",      
				"https://i.imgur.com/",               "i.imgur.com")
