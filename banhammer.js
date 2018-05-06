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
    "an.yandex.ru",
    "direct.yandex.ru",
    "awaps.yandex.net",
    "awaps.yandex.ru",
    "yastatic.net",
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
	"trafficfactory.biz"

];

var keyword_filters =
[
	"advert",
	"bn_",
	"!ad_",
	"_adtune",
	"yap-",
	"adv_",
	"-ads-",
	"-ad-",
	"adfox_",
	"_ads_",
	"adsbygoogle",
	"advert",
	"ban_"
];

var ads_count = 0;
var total_ads_count = 0;
var total_blocking_time = 0;

function checkUrlInSrc(ad, url_filter)
{
	return ad.src.toLowerCase().indexOf(url_filters[j]) > -1;
}

function checkUrlInSrcName(ad, url_filter)
{
	return checkUrlInSrc(ad, url_filter) || ad.name.toLowerCase().indexOf(url_filter) > -1;
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

function blockAd_a(ad)
{
    for (j = 0; j < url_filters.length; j++)
    {
        if (ad.href.toLowerCase().indexOf(url_filters[j]) > -1)
        {
            console.log("blockAd_a " + ad.href);
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
        console.log("blockAd_a Яндекс.Директ");
        ad.remove();
        ads_count++;
    }
}

function blockAd_frame(ad)
{
    for (j = 0; j < url_filters.length; j++)
    {
        if (checkUrlInSrcName(ad, url_filters[j]))
        {
            console.log("blockAd_frame " + ad.src + " " + ad.name);
            ad.remove();
            ads_count++;
            return;
        }
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
                console.log("blockAd_div " + ad.id + " " + ad.className);
                ad.style.display = "none";
                ads_count++;
            }
            return;
        }
    }
}

function blockAd_script(ad)
{
    for (j = 0; j < keyword_filters.length; j++)
    {
		if (checkKeywordInIdClass(ad, keyword_filters[j]))
        {
            console.log("blockAd_script " + ad.id + " " + ad.className);
            ad.remove();
            ads_count++;
            return;
        }
    }
    for (j = 0; j < url_filters.length; j++)
    {
        if (checkUrlInSrc(ad, url_filters[j]))
        {
            console.log("blockAd_script " + ad.src);
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
    if (ads_count > 0)
    {
		var endTime = new Date();
		if (total_ads_count == 0)
		{
			console.log("BanHammer: start ads blocking");
		}
		total_ads_count += ads_count;
		total_blocking_time += endTime - startTime;
        console.log("BanHammer: total " + total_ads_count + " ads removed, time " + total_blocking_time + " ms");
    }
}, 1000);
