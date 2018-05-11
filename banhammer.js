// ==UserScript==
// @name         BanHammer
// @namespace    https://gist.github.com/mistificator
// @version      0.1
// @description  BanHammer bans ads and banners by hardcoded banlist. Ban, ban, ban!!!11
// @author       Mist Poryvaev
// @match       *://*/*
// @run-at      document-end
// ==/UserScript==

var url_filters =
[
	".ru/click",
	".com/click",
    "an.yandex.",
    "direct.yandex.",
    "awaps.yandex.",
    "yastatic.net",
	".yandex.ru/count",
    "banner.rbc.ru",
    "medialand.ru",
	"adx.com.ru",
	"ssp.rambler.ru",
	"dsp-rambler.ru",
	"dsp.rambler.ru",
	"recreativ.ru",
	".everesttech.net",
	".rubiconproject.com",
	".hybrid.ai",
	"googletagmanager.com",
    ".doubleclick.net",
    "partner.googleadservices.com",
    ".googlesyndication.com",
    ".google-analytics.com",
    "creative.ak.fbcdn.net",
    ".adbrite.com",
    ".exponential.com",
    ".quantserve.com",
    ".scorecardresearch.com",
    ".zedo.com",
    "banana.plurk.com",
    "ads.simplepath.com",
    "exoclick.com",
	"mrelko.com",
	"trafficfactory.biz",
	"leokross.com",
	"fer-ta.com",
	"advert",
	"adfox",
	"bingads.com",
	"adnxs.com",
	"reklama",
	"!ad.",
	"tns-counter.",
	"radar.imgsmail.ru"
];

var keyword_filters =
[
	"advert",
	"bn_",
	"!ad_",
	"_adtune",
	"yap-",
	"yandex_rtb",
	"adv_",
	"-ads-",
	"-ad-",
	"adfox",
	"_ads_",
	"adsbygoogle",
	"advert",
	"ban_",
	"-adv-",
	"adsense",
	"adbox",
	"sponsor",
	"revexit",
	"rcjsload",
	"aswift",
	"widead",
	"adcont",
	"mailru",
	"counter",
	"reklama"
];

var style_exception_filters =
[
	"yandex.",
	"ya.ru",
	"mail.",
	"google.",
	"rambler."
];

var ads_count = 0;
var total_ads_count = 0;
var total_blocking_time = 0;

function checkHost(url_filter)
{
	return !window.location.hostname || window.location.hostname.toLowerCase().indexOf(url_filter) == -1;
}

function checkHosts(url_filters)
{
	for (j = 0; j < url_filters.length; j++)
	{
		if (!checkHost(url_filters[j]))
		{
			return false;
		}
	}
	return true;
}

function checkUrlInSrc(ad, url_filter)
{
	var starts_with = false;
	if (url_filter.charAt(0) == '!')
	{
		url_filter = url_filter.substr(1);
		starts_with = true;
	}
	return checkHost(url_filter) && ((starts_with && ad.src.toLowerCase().indexOf(url_filters[j]) == 0) || (!starts_with && ad.src.toLowerCase().indexOf(url_filters[j]) > -1));
}

function checkUrlInSrcName(ad, url_filter)
{
	var starts_with = false;
	if (url_filter.charAt(0) == '!')
	{
		url_filter = url_filter.substr(1);
		starts_with = true;
	}
	return checkHost(url_filter) && (checkUrlInSrc(ad, url_filter) || ((starts_with && ad.name.toLowerCase().indexOf(url_filter) == 0) || (starts_with && ad.name.toLowerCase().indexOf(url_filter) > -1)));
}

function checkKeywordInIdClass(ad, keyword_filter)
{
	var starts_with = false;
	if (keyword_filter.charAt(0) == '!')
	{
		keyword_filter = keyword_filter.substr(1);
		starts_with = true;
	}
	return ((starts_with && (ad.id.toLowerCase().indexOf(keyword_filter) == 0 || ad.className.toLowerCase().indexOf(keyword_filter) == 0)) ||
		(!starts_with && (ad.id.toLowerCase().indexOf(keyword_filter) > -1 || ad.className.toLowerCase().indexOf(keyword_filter) > -1)));
}

function checkKeywordInText(ad, keyword_filter)
{
	return ad.textContent.toLowerCase().indexOf(keyword_filter) > -1;
}

function checkUrlInText(ad, url_filter)
{
	return checkHost(url_filter) && checkKeywordInText(ad, url_filter);
}

function blockAd_a(ad)
{
    for (j = 0; j < url_filters.length; j++)
    {
        if (ad.href.toLowerCase().indexOf(url_filters[j]) > -1)
        {
            console.log("BanHammer: a " + ad.href + " filter " + url_filters[j]);
            ad.remove();
            ads_count++;
            return;
        }
    }
    for (j = 0; j < keyword_filters.length; j++)
    {
		if (checkKeywordInIdClass(ad, keyword_filters[j]))
        {
            console.log("BanHammer: a " + ad.id + " " + ad.className + " filter " + keyword_filters[j]);
            ad.remove();
            ads_count++;
            return;
        }
    }	
    if (ad.textContent.indexOf("Янд") > -1 && ad.textContent.indexOf(".Ди") > -1) // you are fucking crazy bastards
    {
        ad = ad.parentNode.parentNode.parentNode;
        if (ad.tagName.toLowerCase() != "span" || ad.tagName.toLowerCase() != "div")
        {
            ad = ad.parentNode;
        }
        console.log("BanHammer: Яндекс.Директ");
        ad.remove();
        ads_count++;
		return;
    }	
	if (ad.href.split("/").length > 16) // you are fucking crazy bastards 
	{
        console.log("BanHammer: a href has too many levels");
        ad.remove();
        ads_count++;
		return;		
	}
}

function blockAd_frame(ad)
{
    for (j = 0; j < url_filters.length; j++)
    {
        if (checkUrlInSrcName(ad, url_filters[j]))
        {
            console.log("BanHammer: frame " + ad.src + " " + ad.name + " filter " + url_filters[j]);
            ad.remove();
            ads_count++;
            return;
        }
    }
	if (ad.src.split("/").length > 16) // you are fucking crazy bastards 
	{
        console.log("BanHammer: frame src has too many levels");
        ad.remove();
        ads_count++;
		return;		
	}	
}

function blockAd_div(ad)
{
    for (j = 0; j < keyword_filters.length; j++)
    {
		if (checkKeywordInIdClass(ad, keyword_filters[j]))
        {
            // ad.remove(); // this is the cause of hanging of some sites
            if (ad.style.display != "none")
            {
                console.log("BanHammer: div " + ad.id + " " + ad.className + " filter " + keyword_filters[j]);
                ad.style.display = "none";
				ad.innerHtml = "";
                ads_count++;
            }
            return;
        }
    }
	if (ad.style.backgroundImage.split("/").length > 16) // you are fucking crazy bastards 
	{
        console.log("BanHammer: div background-image has too many levels");
        ad.remove();
        ads_count++;
		return;		
	}		
}

function blockAd_script(ad)
{
    for (j = 0; j < keyword_filters.length; j++)
    {
		if (checkUrlInSrc(ad, keyword_filters[j]) || checkKeywordInIdClass(ad, keyword_filters[j]))
        {
            console.log("BanHammer: script " + ad.src + " " + ad.id + " " + ad.className + " filter " + keyword_filters[j]);
/*
			if (ad.parentNode.tagName.toLowerCase() != "body" && ad.parentNode.tagName.toLowerCase() != "head")				
			{
				ad = ad.parentNode;
			}
*/			
            ad.remove();
            ads_count++;
            return;
        }
    }
    for (j = 0; j < url_filters.length; j++)
    {
        if (checkUrlInSrc(ad, url_filters[j]) || checkUrlInText(ad, url_filters[j]))
        {
            console.log("BanHammer: script " + ad.src + " filter " + url_filters[j]);
            ad.remove();
            ads_count++;
            return;
        }
    }
}

function blockAd_style(ad)
{
    for (j = 0; j < url_filters.length; j++)
    {
        if (checkUrlInText(ad, url_filters[j]))
        {
            console.log("BanHammer: style " + ad.textContent + " filter " + url_filters[j]);
            ad.remove();
            ads_count++;
            return;
        }
    }
}

var timer_id = setInterval(function()
{
	ads_count = 0;
	var startTime = new Date();
    var links = document.getElementsByTagName("A");
    for (i = 0; i < links.length; i++)
    {
        blockAd_a(links[i]);
    }
    var frames = document.getElementsByTagName("IFRAME");
    for (i = 0; i < frames.length; i++)
    {
        blockAd_frame(frames[i]);
    }
    var divs = document.getElementsByTagName("DIV");
    for (i = 0; i < divs.length; i++)
    {
        blockAd_div(divs[i]);
    }
    var inss = document.getElementsByTagName("INS");
    for (i = 0; i < inss.length; i++)
    {
        blockAd_div(inss[i]);
    }
    var scripts = document.getElementsByTagName("SCRIPT");
    for (i = 0; i < scripts.length; i++)
    {
        blockAd_script(scripts[i]);
    }
	if (checkHosts(style_exception_filters))
	{
		var styles = document.getElementsByTagName("STYLE");
		for (i = 0; i < styles.length; i++)
		{
			blockAd_style(styles[i]);
		}		
	}
    if (ads_count > 0)
    {
		var endTime = new Date();
		total_ads_count += ads_count;
		total_blocking_time += endTime - startTime;
        console.log("BanHammer: total " + total_ads_count + " ads removed, time " + total_blocking_time + " ms");
    }
}, 1000);
