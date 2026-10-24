// ==UserScript==
// @name         BanHammer
// @namespace    https://gist.github.com/mistificator
// @version      2.0
// @description  BanHammer 2.0: CSS shield, Fetch/XHR interceptor, ERID & MutationObserver adblocker
// @author       Mist Poryvaev
// @match        *://*/*
// @run-at       document-end
// ==/UserScript==

(function() {
    if (window.__banhammer_loaded) {
        return;
    }
    window.__banhammer_loaded = true;

    var url_filters = [
        ".ru/click",
        ".com/click",
        "an.yandex.",
        "direct.yandex.",
        "awaps.yandex.",
        "bs.yandex.",
        "yabs.yandex.",
        "mc.yandex.",
        "adsdk.yandex.",
        "yandexadexchange.",
        "yastatic.net/pcode",
        "yastatic.net/partner-code",
        ".yandex.ru/count",
        ".yandex.ru/ads",
        "ord.yandex.",
        "ord.vk.",
        "ord.ozon.",
        "banner.rbc.ru",
        "medialand.ru",
        "adx.com.ru",
        "ssp.rambler.ru",
        "dsp-rambler.ru",
        "dsp.rambler.ru",
        "recreativ.ru",
        "buzzoola.com",
        "relap.io",
        "smi2.ru/data/js",
        "smi2.net",
        "24smi.net",
        "lentainform.com",
        "mgid.com",
        "marketgid.com",
        "directadvert.ru",
        "teasernet.com",
        "kadam.net",
        "adriver.ru",
        "adhigh.net",
        "otm-r.com",
        " Hybrid.ai",
        ".hybrid.ai",
        "betweendigital.com",
        "sape.ru",
        "rtb.sape.ru",
        "myTarget",
        "ad.mail.ru",
        "target.my.com",
        "top-fwz1.mail.ru",
        ".everesttech.net",
        ".rubiconproject.com",
        "googletagmanager.com",
        "googletagservices.com",
        ".doubleclick.net",
        ".googleadservices.com",
        ".googlesyndication.com",
        ".google-analytics.com",
        "pagead2.googlesyndication.com",
        "adservice.google.",
        "creative.ak.fbcdn.net",
        ".criteo.com",
        ".criteo.net",
        ".taboola.com",
        ".outbrain.com",
        ".pubmatic.com",
        ".openx.net",
        ".casalemedia.com",
        ".smartadserver.com",
        ".adbrite.com",
        ".exponential.com",
        ".quantserve.com",
        ".scorecardresearch.com",
        ".zedo.com",
        "ads.simplepath.com",
        "exoclick.com",
        "juicyads.com",
        "popads.net",
        "propellerads.com",
        "trafficjunky.net",
        "trafficfactory.biz",
        "adnium.com",
        "traffichaus.com",
        "bingads.com",
        "adnxs.com",
        "tns-counter.",
        "radar.imgsmail.ru",
        "yadro.ru",
        "adfox.ru",
        "adfox.yandex.ru",
        "/clickad?",
        "/banners.php?",
        "erid="
    ];

    var keyword_filters = [
        "$movie_player", // youtube exception
        "yandex_rtb",
        "adfox_",
        "ya-partner",
        "yap-",
        "adv_",
        "_adtune",
        "-ads-",
        "-ad-",
        "!ad_",
        "!ads_",
        "!ad-",
        "!ads-",
        "_ads_",
        "_ads!",
        "_ad!",
        "-ads!",
        "-ad!",
        "adsbygoogle",
        "google_ads",
        "div-gpt-ad",
        "advert",
        "bn_",
        "ban_",
        "-adv-",
        "adsense",
        "adbox",
        "adslot",
        "adunit",
        "ad-banner",
        "ad_banner",
        "ad-wrapper",
        "ad-container",
        "sponsor",
        "revexit",
        "rcjsload",
        "aswift",
        "widead",
        "adcont",
        "reklama",
        "adhiding",
        "mgbox",
        "taboola",
        "outbrain",
        "criteo",
        "buzzoola",
        "relap"
    ];

    var style_exception_filters = [
        "yandex.",
        "ya.ru",
        "mail.",
        "google.",
        "rambler.",
        "vk.com",
        "youtube.com"
    ];

    // 1. Косметический CSS-щит (блокирует отрисовку рекламных слотов мгновенно)
    var cosmetic_selectors = [
        '[id^="yandex_rtb"]',
        '[id*="yandex_rtb_"]',
        '[id^="adfox_"]',
        '[class*="ya-partner"]',
        '[class*="yap-layout"]',
        'ins.adsbygoogle',
        '[id^="div-gpt-ad"]',
        'iframe[id^="google_ads"]',
        'iframe[src*="doubleclick.net"]',
        'iframe[src*="googlesyndication.com"]',
        'iframe[src*="an.yandex."]',
        'iframe[src*="adfox"]',
        '[data-google-query-id]',
        '[data-ad-client]',
        '[data-ad-slot]',
        '[data-ad-unit]',
        '[data-Banner-id]',
        '[data-banner-id]',
        '[data-testid="ad-banner"]',
        '[data-qa="ad-banner"]',
        '[id^="taboola-"]',
        '.trc_related_container',
        '.OUTBRAIN',
        '[id^="criteo-"]',
        '[class*="mgbox"]',
        '[id*="ScriptRoot"]',
        '[id^="smi2adblock"]',
        '[class*="relap-"]',
        '[class*="buzzoola"]',
        'a[href*="erid="]',
        'a[href*="ord.yandex.ru"]',
        'a[href*="an.yandex.ru/count"]'
    ];

    function injectCosmeticCss() {
        var head = document.head || document.documentElement;
        if (!head || document.getElementById("banhammer-cosmetic")) {
            return;
        }
        var style = document.createElement("style");
        style.id = "banhammer-cosmetic";
        style.textContent = cosmetic_selectors.join(",\n") +
            " { display: none !important; visibility: hidden !important; height: 0 !important; min-height: 0 !important; max-height: 0 !important; overflow: hidden !important; opacity: 0 !important; pointer-events: none !important; }";
        head.appendChild(style);
    }

    var ads_count = 0;
    var total_ads_count = 0;
    var total_blocking_time = 0;
    var skip_filters = false;

    function checkContains(ad, filter) {
        if (!ad || typeof ad !== "string") {
            return false;
        }
        var lowerAd = ad.toLowerCase();
        if (filter.charAt(0) === '$') {
            filter = filter.substr(1).toLowerCase();
            if (lowerAd.indexOf(filter) > -1) {
                skip_filters = true;
                return false;
            }
            return false;
        }
        if (filter.charAt(0) === '!') {
            filter = filter.substr(1).toLowerCase();
            return lowerAd.indexOf(filter) === 0 || lowerAd.indexOf(" " + filter) > -1;
        }
        if (filter.charAt(filter.length - 1) === '!') {
            filter = filter.substr(0, filter.length - 1).toLowerCase();
            return lowerAd.length >= filter.length &&
                   (lowerAd.indexOf(filter) === (lowerAd.length - filter.length) || lowerAd.indexOf(filter + " ") > -1);
        }
        return lowerAd.indexOf(filter.toLowerCase()) > -1;
    }

    function matchesAnyUrlFilter(url) {
        if (!url || typeof url !== "string") {
            return false;
        }
        var lower = url.toLowerCase();
        for (var i = 0; i < url_filters.length; i++) {
            var f = url_filters[i].replace(/^[!$]/, "").replace(/!$/, "").toLowerCase();
            if (f && lower.indexOf(f) > -1) {
                return true;
            }
        }
        return false;
    }

    // 2. Перехват сетевых запросов (Fetch, XHR, sendBeacon) к рекламным сетям
    try {
        if (window.fetch) {
            var origFetch = window.fetch;
            window.fetch = function(input, init) {
                var url = (typeof input === "string") ? input : (input && input.url ? input.url : "");
                if (matchesAnyUrlFilter(url)) {
                    total_ads_count++;
                    return Promise.resolve(new Response("", { status: 204, statusText: "Blocked by BanHammer" }));
                }
                return origFetch.apply(this, arguments);
            };
        }
        if (window.XMLHttpRequest && window.XMLHttpRequest.prototype.open) {
            var origOpen = window.XMLHttpRequest.prototype.open;
            var origSend = window.XMLHttpRequest.prototype.send;
            window.XMLHttpRequest.prototype.open = function(method, url) {
                this.__bh_blocked = matchesAnyUrlFilter(String(url || ""));
                if (this.__bh_blocked) {
                    total_ads_count++;
                    return origOpen.call(this, method, "data:text/plain,");
                }
                return origOpen.apply(this, arguments);
            };
            window.XMLHttpRequest.prototype.send = function() {
                if (this.__bh_blocked) {
                    return;
                }
                return origSend.apply(this, arguments);
            };
        }
        if (navigator && navigator.sendBeacon) {
            var origBeacon = navigator.sendBeacon;
            navigator.sendBeacon = function(url, data) {
                if (matchesAnyUrlFilter(String(url || ""))) {
                    total_ads_count++;
                    return true;
                }
                return origBeacon.apply(this, arguments);
            };
        }
    } catch (e) {}

    function checkHost(url_filter) {
        return Boolean(window.location.hostname) &&
               window.location.hostname !== "" &&
               !checkContains(window.location.hostname, url_filter);
    }

    function checkHosts(filters) {
        for (var j = 0; j < filters.length; j++) {
            if (!checkHost(filters[j])) {
                return false;
            }
        }
        return true;
    }

    function checkUrlInSrc(ad, url_filter) {
        return checkHost(url_filter) && checkContains(ad.src || "", url_filter);
    }

    function checkUrlInSrcName(ad, url_filter) {
        return checkContains(ad.src || "", url_filter) || checkContains(ad.name || "", url_filter);
    }

    function getClassString(ad) {
        if (!ad) return "";
        if (typeof ad.className === "string") return ad.className;
        if (ad.getAttribute) return ad.getAttribute("class") || "";
        return "";
    }

    function checkKeywordInIdClass(ad, keyword_filter) {
        return checkContains(ad.id || "", keyword_filter) || checkContains(getClassString(ad), keyword_filter);
    }

    function checkKeywordInText(ad, keyword_filter) {
        return checkContains(ad.textContent || "", keyword_filter);
    }

    function checkUrlInText(ad, url_filter) {
        return checkHost(url_filter) && checkKeywordInText(ad, url_filter);
    }

    function hideElementSafely(el) {
        if (!el || !el.style) return;
        if (el.style.display !== "none") {
            el.style.setProperty("display", "none", "important");
            el.innerHTML = "";
            ads_count++;
        }
    }

    function climbAndHideAdCard(node) {
        var cur = node;
        for (var depth = 0; depth < 5 && cur && cur.parentElement; depth++) {
            var parent = cur.parentElement;
            if (parent === document.body || parent === document.documentElement) {
                break;
            }
            var rect = parent.getBoundingClientRect ? parent.getBoundingClientRect() : null;
            if (rect && (rect.width > window.innerWidth * 0.85 && rect.height > window.innerHeight * 0.8)) {
                break;
            }
            cur = parent;
        }
        hideElementSafely(cur);
    }

    function blockAd_a(ad) {
        if (!ad) return;
        skip_filters = false;
        var href = ad.href || ad.getAttribute("href") || "";

        // Маркировка рекламы в РФ (erid) — скрываем карточку объявления целиком
        if (href.indexOf("erid=") > -1 || href.indexOf("ord.yandex.") > -1) {
            climbAndHideAdCard(ad);
            return;
        }

        for (var j = 0; j < url_filters.length && !skip_filters; j++) {
            if (checkContains(href, url_filters[j])) {
                ad.remove();
                ads_count++;
                return;
            }
        }
        for (var k = 0; k < keyword_filters.length && !skip_filters; k++) {
            if (checkKeywordInIdClass(ad, keyword_filters[k])) {
                ad.remove();
                ads_count++;
                return;
            }
        }
        if (href.split("/").length > 16) {
            ad.remove();
            ads_count++;
        }
    }

    function blockAd_frame(ad) {
        if (!ad) return;
        skip_filters = false;
        var src = ad.src || ad.getAttribute("src") || "";
        for (var j = 0; j < url_filters.length && !skip_filters; j++) {
            if (checkUrlInSrcName(ad, url_filters[j])) {
                ad.remove();
                ads_count++;
                return;
            }
        }
        if (src.split("/").length > 16) {
            ad.remove();
            ads_count++;
        }
    }

    function blockAd_div(ad) {
        if (!ad || !ad.style) return;
        if (ad.style.display === "none") return;

        if (ad.hasAttribute("data-google-query-id") ||
            ad.hasAttribute("data-ad-client") ||
            ad.hasAttribute("data-ad-slot") ||
            ad.hasAttribute("data-banner-id")) {
            hideElementSafely(ad);
            return;
        }

        skip_filters = false;
        for (var j = 0; j < keyword_filters.length && !skip_filters; j++) {
            if (checkKeywordInIdClass(ad, keyword_filters[j])) {
                hideElementSafely(ad);
                return;
            }
        }

        if (ad.style.backgroundImage && ad.style.backgroundImage.split("/").length > 16) {
            hideElementSafely(ad);
        }
    }

    function blockAd_script(ad) {
        if (!ad) return;
        skip_filters = false;
        for (var j = 0; j < keyword_filters.length && !skip_filters; j++) {
            if (checkUrlInSrc(ad, keyword_filters[j]) || checkKeywordInIdClass(ad, keyword_filters[j])) {
                ad.remove();
                ads_count++;
                return;
            }
        }
        for (var k = 0; k < url_filters.length && !skip_filters; k++) {
            if (checkUrlInSrc(ad, url_filters[k]) || checkUrlInText(ad, url_filters[k])) {
                ad.remove();
                ads_count++;
                return;
            }
        }
    }

    function blockAd_style(ad) {
        if (!ad || ad.id === "banhammer-cosmetic") return;
        skip_filters = false;
        for (var j = 0; j < url_filters.length && !skip_filters; j++) {
            if (checkUrlInText(ad, url_filters[j])) {
                ad.remove();
                ads_count++;
                return;
            }
        }
    }

    function runSweep() {
        injectCosmeticCss();
        ads_count = 0;
        var startTime = new Date();

        var links = document.getElementsByTagName("A");
        for (var i = links.length - 1; i >= 0; i--) { blockAd_a(links[i]); }

        var frames = document.getElementsByTagName("IFRAME");
        for (var i = frames.length - 1; i >= 0; i--) { blockAd_frame(frames[i]); }

        var divs = document.getElementsByTagName("DIV");
        for (var i = divs.length - 1; i >= 0; i--) { blockAd_div(divs[i]); }

        var asides = document.getElementsByTagName("ASIDE");
        for (var i = asides.length - 1; i >= 0; i--) { blockAd_div(asides[i]); }

        var inss = document.getElementsByTagName("INS");
        for (var i = inss.length - 1; i >= 0; i--) { blockAd_div(inss[i]); }

        var scripts = document.getElementsByTagName("SCRIPT");
        for (var i = scripts.length - 1; i >= 0; i--) { blockAd_script(scripts[i]); }

        if (checkHosts(style_exception_filters)) {
            var styles = document.getElementsByTagName("STYLE");
            for (var i = styles.length - 1; i >= 0; i--) { blockAd_style(styles[i]); }
        }

        if (ads_count > 0) {
            var endTime = new Date();
            total_ads_count += ads_count;
            total_blocking_time += (endTime - startTime);
            console.log("BanHammer: total " + total_ads_count + " ads blocked, time " + total_blocking_time + " ms");
        }
    }

    // Первичный проход
    runSweep();

    // 3. Реактивное отслеживание динамической подгрузки через MutationObserver
    var mutationPending = false;
    if (window.MutationObserver && document.documentElement) {
        var observer = new MutationObserver(function() {
            if (!mutationPending) {
                mutationPending = true;
                setTimeout(function() {
                    mutationPending = false;
                    runSweep();
                }, 250);
            }
        });
        observer.observe(document.documentElement, { childList: true, subtree: true });
    }

    // Резервный таймер
    setInterval(runSweep, 2000);
})();