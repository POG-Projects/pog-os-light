#pragma once
#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="fr"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#07080d"><link rel="icon" type="image/png" href="/icon.png"><link rel="apple-touch-icon" href="/icon.png"><link rel="manifest" href="/manifest.webmanifest">
<title>PogLight</title>
<style>
:root{color-scheme:dark;--night:#07080d;--glass:rgba(25,27,36,.57);--glass-strong:rgba(31,34,44,.78);--line:rgba(255,255,255,.11);--line-hi:rgba(255,255,255,.2);--text:#f7f7fa;--muted:#a7a8b3;--soft:#737581;--emerald:#54e7ad;--violet:#a89cff;--danger:#ff7979;--shadow:0 30px 90px rgba(0,0,0,.45);--radius:28px;--font:-apple-system,BlinkMacSystemFont,"SF Pro Display","Inter",Segoe UI,sans-serif;--mono:"SF Mono",ui-monospace,Menlo,monospace}
*{box-sizing:border-box}
html{max-width:100%;overflow-x:clip;background:var(--night);scroll-behavior:smooth;overscroll-behavior-x:none}
body{position:relative;isolation:isolate;margin:0;max-width:100%;min-height:100vh;overflow-x:clip;overscroll-behavior-x:none;color:var(--text);font-family:var(--font);font-size:14px;letter-spacing:-.01em;padding-bottom:52px;background:var(--night)}
body:before{content:"";position:fixed;z-index:-1;inset:0;pointer-events:none;background:
url("data:image/svg+xml,%3Csvg viewBox='0 0 180 180' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='.9' numOctaves='2' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)' opacity='.035'/%3E%3C/svg%3E"),
radial-gradient(70% 55% at 8% -5%,rgba(92,231,178,.16),transparent 70%),
radial-gradient(65% 52% at 96% 5%,rgba(147,125,255,.18),transparent 72%),
linear-gradient(155deg,#11131d 0%,#07080d 48%,#0c0d14 100%);transform:translateZ(0)}
button,input,select{font:inherit}
button,input,select{outline:none}
button:focus-visible,input:focus-visible,select:focus-visible{box-shadow:0 0 0 3px rgba(84,231,173,.28);border-color:var(--emerald)}
.wrap{width:min(1120px,100%);margin:0 auto;padding:26px 24px}
header{display:flex;align-items:center;gap:13px;margin:0 2px 28px}
.logo{position:relative;width:38px;height:38px;border-radius:13px;background:#071518 url("/icon.png") center/cover no-repeat;border:1px solid rgba(255,255,255,.22);box-shadow:inset 0 1px 0 rgba(255,255,255,.22),0 10px 35px rgba(84,231,173,.16)}
.brand{font-size:17px;font-weight:650;letter-spacing:-.02em}
.sub{color:var(--muted);font-size:11px;letter-spacing:.08em;margin-top:2px}
.chip{margin-left:auto;display:flex;align-items:center;gap:8px;border:1px solid var(--line);border-radius:999px;padding:8px 13px;color:var(--muted);font:11px var(--mono);background:rgba(255,255,255,.045);backdrop-filter:blur(18px);-webkit-backdrop-filter:blur(18px)}
.chip:before{content:"";width:7px;height:7px;border-radius:50%;background:var(--emerald);box-shadow:0 0 12px var(--emerald)}
.hero{position:relative;overflow:hidden;min-height:272px;margin-bottom:18px;padding:34px 36px 30px;border:1px solid var(--line-hi);border-radius:34px;background:linear-gradient(135deg,rgba(38,43,53,.68),rgba(18,19,27,.52));box-shadow:var(--shadow),inset 0 1px 0 rgba(255,255,255,.1);backdrop-filter:blur(30px) saturate(125%);-webkit-backdrop-filter:blur(30px) saturate(125%)}
.hero:before,.hero:after{content:"";position:absolute;border-radius:50%;filter:blur(5px);pointer-events:none}
.hero:before{width:320px;height:320px;right:-100px;top:-160px;background:radial-gradient(circle,rgba(168,156,255,.28),transparent 66%)}
.hero:after{width:260px;height:260px;left:25%;bottom:-210px;background:radial-gradient(circle,rgba(84,231,173,.28),transparent 67%)}
.eyebrow{position:relative;z-index:1;color:var(--emerald);font:10px var(--mono);letter-spacing:.15em;text-transform:uppercase;margin-bottom:13px}
h1{position:relative;z-index:1;font-size:clamp(38px,6vw,68px);line-height:.98;letter-spacing:-.065em;font-weight:600;margin:0;max-width:670px}
.hero-copy{position:relative;z-index:1;color:var(--muted);font-size:15px;line-height:1.55;max-width:490px;margin:17px 0 25px}
.light-stage{position:relative;z-index:1;padding:16px 18px;border:1px solid rgba(255,255,255,.1);border-radius:20px;background:rgba(3,4,8,.42);box-shadow:inset 0 1px 10px rgba(0,0,0,.3)}
.strip{display:flex;align-items:center;justify-content:space-between;gap:3px;min-height:18px}
.strip>i{flex:0 1 14px;width:14px;max-width:14px;aspect-ratio:1;border-radius:50%;background:#20222b;box-shadow:inset 0 1px 1px rgba(255,255,255,.08);transition:background .3s,box-shadow .3s}
.layout{display:grid;grid-template-columns:minmax(0,1.45fr) minmax(300px,.75fr);gap:18px;align-items:start;min-width:0}
.stack{display:grid;gap:18px;width:100%;min-width:0}
.card{position:relative;min-width:0;max-width:100%;background:var(--glass);border:1px solid var(--line);border-radius:var(--radius);padding:25px;box-shadow:0 20px 55px rgba(0,0,0,.22),inset 0 1px 0 rgba(255,255,255,.075);backdrop-filter:blur(26px) saturate(125%);-webkit-backdrop-filter:blur(26px) saturate(125%)}
.card-head{display:flex;align-items:flex-start;justify-content:space-between;gap:18px;margin-bottom:20px}
.card h2{font-size:20px;letter-spacing:-.035em;font-weight:600;margin:0}
.kicker{font:9px var(--mono);letter-spacing:.14em;text-transform:uppercase;color:var(--soft);margin-top:5px}
.grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:8px}
.pat{display:flex;align-items:center;gap:9px;min-width:0;background:rgba(5,6,10,.33);border:1px solid rgba(255,255,255,.075);color:#d8d8df;border-radius:14px;padding:11px 12px;font-size:12px;cursor:pointer;text-align:left;transition:transform .18s ease,border-color .18s,background .18s,color .18s}
.pat:before{content:"";width:6px;height:6px;flex:0 0 auto;border-radius:50%;background:#5e606b;transition:.18s}
.pat:hover{transform:translateY(-1px);border-color:var(--line-hi);background:rgba(255,255,255,.07);color:#fff}
.pat.on{color:#fff;border-color:rgba(84,231,173,.4);background:linear-gradient(135deg,rgba(84,231,173,.16),rgba(168,156,255,.08));box-shadow:inset 0 1px 0 rgba(255,255,255,.08),0 8px 25px rgba(84,231,173,.08)}
.pat.on:before{background:var(--emerald);box-shadow:0 0 12px var(--emerald)}
.scene-controls{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:18px}
.color-control{display:flex;align-items:center;gap:12px;padding:12px 14px;border-radius:17px;border:1px solid rgba(255,255,255,.075);background:rgba(5,6,10,.26)}
.color-control input[type=color]{appearance:none;-webkit-appearance:none;width:40px;height:40px;padding:0;border:0;background:none;cursor:pointer}
.color-control input[type=color]::-webkit-color-swatch-wrapper{padding:0}
.color-control input[type=color]::-webkit-color-swatch{border:1px solid rgba(255,255,255,.32);border-radius:50%;box-shadow:0 5px 18px currentColor}
.control-title{display:block;font-size:12px;color:var(--muted)}
.control-value{display:block;font:11px var(--mono);color:var(--text);margin-top:3px;text-transform:uppercase}
.sliders{grid-column:1/-1;display:grid;gap:8px;margin-top:3px}
.row{display:flex;align-items:center;gap:13px;min-width:0;min-height:42px;margin:5px 0}
.row label{color:var(--muted);font-size:12px;min-width:105px}
.val{min-width:46px;text-align:right;color:var(--text);font:11px var(--mono)}
input[type=range]{appearance:none;-webkit-appearance:none;flex:1;height:4px;border-radius:99px;background:rgba(255,255,255,.13);cursor:pointer}
input[type=range]::-webkit-slider-thumb{appearance:none;-webkit-appearance:none;width:18px;height:18px;border-radius:50%;background:#fff;border:4px solid rgba(255,255,255,.25);box-shadow:0 2px 12px rgba(0,0,0,.45)}
select,input[type=text],input[type=password],input[type=number]{min-width:0;flex:1;background:rgba(5,6,10,.38);border:1px solid rgba(255,255,255,.1);border-radius:13px;color:var(--text);padding:11px 12px;font-size:12px;transition:border-color .18s,background .18s}
select:hover,input[type=text]:hover,input[type=password]:hover,input[type=number]:hover{border-color:var(--line-hi);background:rgba(5,6,10,.5)}
select{appearance:none;background-image:linear-gradient(45deg,transparent 50%,#898b95 50%),linear-gradient(135deg,#898b95 50%,transparent 50%);background-position:calc(100% - 16px) 50%,calc(100% - 12px) 50%;background-size:4px 4px,4px 4px;background-repeat:no-repeat;padding-right:28px}
input[type=file]{width:100%;min-width:0;color:var(--muted);font-size:11px}
input[type=file]::file-selector-button{border:1px solid var(--line);background:rgba(255,255,255,.06);color:var(--text);border-radius:11px;padding:8px 11px;margin-right:10px;cursor:pointer}
.btn{border:1px solid rgba(255,255,255,.12);border-radius:13px;padding:11px 16px;background:linear-gradient(135deg,#72edbd,#45cf9a);color:#062519;font-weight:650;font-size:12px;cursor:pointer;box-shadow:inset 0 1px 0 rgba(255,255,255,.36),0 8px 24px rgba(84,231,173,.14);transition:transform .18s,filter .18s}
.btn:hover{transform:translateY(-1px);filter:brightness(1.06)}
.btn.ghost{background:rgba(255,255,255,.045);border-color:var(--line);color:var(--text);box-shadow:inset 0 1px 0 rgba(255,255,255,.06)}
.action-row{display:flex;align-items:center;flex-wrap:wrap;gap:10px;min-width:0;margin-top:17px}
.hint{color:var(--soft);font-size:11px;margin:0 0 13px;line-height:1.55}
.divider{height:1px;background:rgba(255,255,255,.075);margin:20px 0}
.pbar{background:rgba(0,0,0,.3);border-radius:99px;height:6px;overflow:hidden;margin-top:12px}
.pbar>div{height:100%;width:0;background:linear-gradient(90deg,var(--emerald),var(--violet));transition:width .2s}
.network-list{display:grid;gap:6px;margin:9px 0}
.network{display:flex;justify-content:space-between;width:100%;border:1px solid rgba(255,255,255,.07);border-radius:12px;padding:9px 11px;background:rgba(5,6,10,.25);color:var(--muted);font-size:11px;cursor:pointer}
.network:hover{color:#fff;border-color:var(--line-hi)}
.section-map{position:relative;height:46px;margin:18px 0 14px;overflow:hidden;border:1px solid rgba(255,255,255,.1);border-radius:15px;background:rgba(3,4,8,.46);box-shadow:inset 0 2px 12px rgba(0,0,0,.35)}
.section-piece{position:absolute;top:6px;bottom:6px;min-width:3px;border-radius:9px;border:1px solid rgba(255,255,255,.22);box-shadow:0 0 18px color-mix(in srgb,var(--zone) 38%,transparent),inset 0 1px 0 rgba(255,255,255,.25);background:linear-gradient(110deg,color-mix(in srgb,var(--zone) 82%,#fff),var(--zone));opacity:.9}
.section-piece.off{filter:saturate(.2);opacity:.28}
.zone-list{display:grid;gap:10px;margin-top:12px}
.zone{padding:15px;border:1px solid rgba(255,255,255,.085);border-radius:19px;background:rgba(5,6,10,.25)}
.zone-head{display:flex;align-items:center;gap:10px;margin-bottom:13px}.zone-dot{width:9px;height:9px;flex:0 0 auto;border-radius:50%;background:var(--zone);box-shadow:0 0 13px var(--zone)}.zone-head input{font-size:14px;font-weight:600;padding:8px 10px}.icon-btn{border:1px solid var(--line);border-radius:11px;background:rgba(255,255,255,.04);color:var(--muted);min-width:38px;height:38px;cursor:pointer}.icon-btn:hover{color:var(--danger);border-color:rgba(255,121,121,.28)}
.zone-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;min-width:0}.zone-field{display:grid;min-width:0;gap:6px;color:var(--soft);font-size:10px}.zone-field.wide{grid-column:1/-1}.zone-field input,.zone-field select{width:100%;min-width:0;max-width:100%}.zone-color{height:43px!important;width:100%!important;padding:4px!important;border:1px solid rgba(255,255,255,.1)!important;border-radius:13px!important;background:rgba(5,6,10,.38)!important}.zone-color::-webkit-color-swatch{border:0;border-radius:9px}
.zone-foot{display:flex;align-items:center;gap:9px;margin-top:12px;color:var(--muted);font-size:11px}.zone-foot input{width:17px;height:17px;accent-color:var(--emerald)}.zone-foot .range-value{margin-left:auto;font:10px var(--mono);color:var(--text)}
.empty-zones{padding:18px;text-align:center;border:1px dashed rgba(255,255,255,.12);border-radius:18px;color:var(--soft);font-size:11px;line-height:1.5}
.peripheral{display:grid;gap:10px;padding:15px;margin:10px 0;border:1px solid rgba(255,255,255,.08);border-radius:19px;background:rgba(5,6,10,.24)}.peripheral-head{display:flex;align-items:center;justify-content:space-between;gap:12px}.peripheral-title{font-size:13px;font-weight:600}.peripheral-copy{color:var(--soft);font-size:10px;line-height:1.45;margin-top:3px}.pin-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}.pin-field{display:grid;gap:6px;min-width:0;color:var(--soft);font-size:10px}.pin-field select{width:100%;min-width:0}.peripheral select:disabled{opacity:.4}.peripheral-status{margin:2px 0 0}
.toast{position:fixed;z-index:20;left:50%;bottom:22px;transform:translate(-50%,12px);background:rgba(239,240,244,.92);color:#16171c;padding:11px 17px;border-radius:14px;font-size:12px;font-weight:650;box-shadow:0 18px 55px rgba(0,0,0,.4);opacity:0;transition:.25s;pointer-events:none;backdrop-filter:blur(20px)}
.toast.show{opacity:1;transform:translate(-50%,0)}
.mobile-nav{display:none}
.onboarding{display:none;position:fixed;z-index:50;inset:0;align-items:center;justify-content:center;padding:20px;background:rgba(4,5,9,.72);backdrop-filter:blur(24px) saturate(120%);-webkit-backdrop-filter:blur(24px) saturate(120%)}
.onboarding.show{display:flex;animation:obFade .35s ease-out}
.ob-shell{position:relative;width:min(480px,100%);min-height:650px;overflow:hidden;display:flex;flex-direction:column;border:1px solid rgba(255,255,255,.18);border-radius:34px;background:linear-gradient(155deg,rgba(37,40,51,.96),rgba(13,14,21,.97));box-shadow:0 40px 120px rgba(0,0,0,.65),inset 0 1px 0 rgba(255,255,255,.13)}
.ob-glow{position:absolute;width:360px;height:360px;top:-210px;right:-130px;border-radius:50%;background:radial-gradient(circle,rgba(168,156,255,.35),rgba(84,231,173,.12) 45%,transparent 70%);pointer-events:none}
.ob-top{position:relative;z-index:1;display:flex;align-items:center;justify-content:space-between;padding:25px 27px 0}
.ob-brand{display:flex;align-items:center;gap:10px;font-size:13px;font-weight:650}.ob-brand .logo{width:30px;height:30px;border-radius:10px}
.ob-skip{border:0;background:none;color:var(--muted);font-size:11px;padding:8px;cursor:pointer}
.ob-progress{display:flex;gap:6px}.ob-progress i{display:block;width:6px;height:6px;border-radius:99px;background:rgba(255,255,255,.16);transition:.25s}.ob-progress i.on{width:22px;background:var(--emerald);box-shadow:0 0 12px rgba(84,231,173,.6)}
.ob-pages{position:relative;z-index:1;flex:1;display:flex}
.ob-page{display:none;width:100%;padding:42px 34px 32px;overflow-y:auto;flex-direction:column}.ob-page.on{display:flex;animation:obSlide .32s ease-out}
.ob-art{position:relative;height:190px;display:flex;align-items:center;justify-content:center;margin:-8px 0 25px}
.ob-orb{width:132px;height:132px;border-radius:50%;background:radial-gradient(circle at 34% 28%,#fff 0 2%,#a7ffe0 8%,var(--emerald) 27%,#527cfa 62%,#231b4f 100%);box-shadow:0 0 35px rgba(84,231,173,.28),0 0 90px rgba(120,101,255,.28);animation:orbFloat 4s ease-in-out infinite}
.ob-art:after{content:"";position:absolute;width:210px;height:32px;bottom:5px;border-radius:50%;background:radial-gradient(ellipse,rgba(84,231,173,.2),transparent 70%);filter:blur(8px)}
.ob-page h2{font-size:36px;line-height:1.02;letter-spacing:-.055em;font-weight:610;margin:0 0 14px}
.ob-page>p{color:var(--muted);font-size:14px;line-height:1.55;margin:0 0 25px}
.ob-fields{display:grid;gap:11px;margin-bottom:16px}
.ob-field{display:grid;gap:7px}.ob-field>span{font-size:11px;color:var(--muted)}
.ob-field input,.ob-field select{width:100%;min-height:48px}
.ob-two{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.ob-wire{display:grid;gap:9px;padding:16px;border:1px solid rgba(255,255,255,.08);border-radius:18px;background:rgba(0,0,0,.16);margin-bottom:20px}
.ob-wire div{display:flex;align-items:center;gap:10px;color:var(--muted);font-size:11px}.ob-wire i{width:28px;height:3px;border-radius:4px}.ob-wire b{color:var(--text);font-weight:550;margin-left:auto}
.ob-wifi-list{max-height:126px;overflow:auto;display:grid;gap:6px;margin:-3px 0 10px}
.ob-poghome{display:flex;gap:11px;align-items:center;padding:13px 14px;border:1px solid rgba(84,231,173,.19);border-radius:16px;background:rgba(84,231,173,.07);margin:5px 0 17px;color:var(--muted);font-size:11px;line-height:1.4}
.ob-poghome i{width:9px;height:9px;flex:0 0 auto;border-radius:50%;background:var(--emerald);box-shadow:0 0 13px var(--emerald)}
.ob-actions{margin-top:auto;display:flex;gap:10px}.ob-actions .btn{flex:1;min-height:48px}.ob-actions .btn.back{flex:0 0 50px;padding:0}
.ob-success{margin:auto 0;text-align:center}.ob-success .ob-orb{width:96px;height:96px;margin:0 auto 35px}.ob-success h2{font-size:34px}.ob-success p{color:var(--muted);line-height:1.55}
@keyframes obFade{from{opacity:0}to{opacity:1}}@keyframes obSlide{from{opacity:0;transform:translateX(14px)}to{opacity:1;transform:none}}@keyframes panelIn{from{opacity:0;transform:translateY(6px)}to{opacity:1;transform:none}}@keyframes orbFloat{0%,100%{transform:translateY(4px) scale(.98)}50%{transform:translateY(-7px) scale(1.02)}}
@media(max-width:850px){.layout{grid-template-columns:1fr}.stack{grid-template-columns:1fr 1fr}.hero{min-height:250px}}
@media(max-width:620px){
body{padding-bottom:calc(102px + env(safe-area-inset-bottom))}
.wrap{padding:calc(16px + env(safe-area-inset-top)) 13px 18px}
header{margin:0 3px 17px}.brand{font-size:15px}.sub{display:none}.chip{max-width:135px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;padding:7px 10px}
.hero{min-height:0;padding:22px 20px 18px;border-radius:25px;margin-bottom:13px}.hero .eyebrow{display:none}.hero h1{font-size:36px;line-height:1;letter-spacing:-.055em}.hero-copy{display:none}.light-stage{margin-top:20px;padding:13px 14px;border-radius:17px}
body.mobile-secondary .hero{display:none}
.layout{display:block}.stack{display:contents}.mobile-panel{display:none}.mobile-panel.mobile-active{display:block;animation:panelIn .22s ease-out}
.card{padding:20px;border-radius:23px;margin-bottom:13px}.card-head{margin-bottom:17px}.card h2{font-size:22px}
.grid{grid-template-columns:repeat(2,minmax(0,1fr));gap:7px}.pat{min-height:46px;padding:10px 11px;font-size:12px}
.scene-controls{grid-template-columns:1fr}.sliders{grid-column:auto}.color-control{min-height:66px}
.row{align-items:flex-start;flex-wrap:wrap;gap:8px;margin:9px 0}.row label{flex:1 1 100%;min-width:0}.row .val{margin-left:auto}.row select,.row input[type=text],.row input[type=password],.row input[type=number]{min-height:48px}.btn{min-height:46px}
.zone{padding:13px}.zone-grid{grid-template-columns:1fr 1fr}.section-map{height:42px}.zone-head input{min-height:42px}
.peripheral{padding:13px}.pin-grid{grid-template-columns:1fr 1fr}
.mobile-nav{position:fixed;z-index:30;display:grid;grid-template-columns:repeat(3,1fr);left:12px;right:12px;bottom:calc(10px + env(safe-area-inset-bottom));height:72px;padding:7px;border:1px solid rgba(255,255,255,.16);border-radius:23px;background:rgba(24,26,34,.82);box-shadow:0 20px 60px rgba(0,0,0,.5),inset 0 1px 0 rgba(255,255,255,.1);backdrop-filter:blur(26px) saturate(145%);-webkit-backdrop-filter:blur(26px) saturate(145%)}
.mobile-nav button{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:5px;border:0;border-radius:17px;background:transparent;color:var(--soft);font-size:10px;cursor:pointer;min-width:0}
.mobile-nav button.active{color:var(--text);background:rgba(255,255,255,.075)}.mobile-nav button.active svg{color:var(--emerald);filter:drop-shadow(0 0 6px rgba(84,231,173,.55))}
.mobile-nav svg{width:20px;height:20px;color:currentColor;stroke:currentColor;fill:none;stroke-width:1.7;stroke-linecap:round;stroke-linejoin:round}
.toast{bottom:calc(94px + env(safe-area-inset-bottom));white-space:nowrap;max-width:calc(100% - 28px);overflow:hidden;text-overflow:ellipsis}
.onboarding{padding:0}.ob-shell{width:100%;height:100%;min-height:0;border:0;border-radius:0;padding-top:env(safe-area-inset-top);padding-bottom:env(safe-area-inset-bottom)}.ob-top{padding:18px 20px 0}.ob-page{padding:28px 22px 22px}.ob-art{height:155px;margin:-5px 0 20px}.ob-orb{width:112px;height:112px}.ob-page h2{font-size:34px}.ob-page>p{font-size:13px;margin-bottom:20px}.ob-two{grid-template-columns:1fr 1fr}.ob-actions{position:sticky;bottom:0;padding-top:12px;background:linear-gradient(transparent,rgba(14,15,22,.97) 22%)}
}
@media(prefers-reduced-motion:reduce){*{scroll-behavior:auto!important;transition:none!important}}
</style></head>
<body>
<div class="onboarding" id="onboarding" role="dialog" aria-modal="true" aria-label="Configuration de PogLight">
  <div class="ob-shell">
    <div class="ob-glow"></div>
    <div class="ob-top">
      <div class="ob-brand"><div class="logo"></div><span>POG Light</span></div>
      <div class="ob-progress" id="obProgress"><i class="on"></i><i></i><i></i></div>
      <button class="ob-skip" type="button" onclick="closeOnboarding()">Plus tard</button>
    </div>
    <div class="ob-pages">
      <section class="ob-page on" data-step="0">
        <div class="ob-art"><div class="ob-orb"></div></div>
        <div class="eyebrow">Bienvenue dans POG</div>
        <h2>Votre lumière.<br>Votre ambiance.</h2>
        <p>Configurons votre bande LED et relions-la à PogHome. Cela prend moins d’une minute.</p>
        <div class="ob-actions"><button class="btn" type="button" onclick="obNext()">Commencer</button></div>
      </section>
      <section class="ob-page" data-step="1">
        <div class="eyebrow">Étape 1 · La bande</div>
        <h2>Parlons de vos LED.</h2>
        <p>Indiquez simplement ce qui est branché au contrôleur. Vous pourrez tout modifier plus tard.</p>
        <div class="ob-fields">
          <div class="ob-two">
            <label class="ob-field"><span>Nombre de LED</span><input type="number" id="obNumLeds" min="1" max="300" value="60"></label>
            <label class="ob-field"><span>Broche de données</span><select id="obLedPin"></select></label>
          </div>
          <label class="ob-field"><span>Type de bande</span><select id="obAnalog"><option value="0">Adressable · WS2812</option><option value="1">Analogique · PWM</option></select></label>
          <label class="ob-field"><span>Utilité principale</span><select id="obPurpose"></select></label>
        </div>
        <div class="ob-wire">
          <div><i style="background:#ff6577"></i>Alimentation <b>5 V externe</b></div>
          <div><i style="background:#71737e"></i>Masse commune <b>GND</b></div>
          <div><i style="background:var(--emerald)"></i>Signal de données <b id="obPinHint">GPIO 2</b></div>
        </div>
        <div class="ob-actions"><button class="btn ghost back" type="button" onclick="obBack()">←</button><button class="btn" type="button" onclick="obNext()">Continuer</button></div>
      </section>
      <section class="ob-page" data-step="2">
        <div class="eyebrow">Étape 2 · La maison</div>
        <h2>Connectez PogLight.</h2>
        <p>Choisissez votre Wi‑Fi. PogHome le détectera automatiquement sur le même réseau.</p>
        <div class="ob-fields">
          <label class="ob-field"><span>Réseau Wi‑Fi</span><div class="ob-two"><input type="text" id="obSsid" placeholder="Nom du réseau"><button class="btn ghost" type="button" onclick="scanOnboardingWifi()">Rechercher</button></div></label>
          <div class="ob-wifi-list" id="obWifiList"></div>
          <label class="ob-field"><span>Mot de passe</span><input type="password" id="obWifiPass" placeholder="••••••••"></label>
        </div>
        <div class="ob-poghome"><i></i><span>Après le redémarrage, PogLight apparaîtra dans les appareils à adopter de PogHome.</span></div>
        <div class="ob-actions"><button class="btn ghost back" type="button" onclick="obBack()">←</button><button class="btn" id="obFinish" type="button" onclick="finishOnboarding()">Connecter et terminer</button></div>
      </section>
      <section class="ob-page" data-step="3">
        <div class="ob-success"><div class="ob-orb"></div><h2>Tout est prêt.</h2><p>PogLight redémarre et rejoint votre Wi‑Fi.<br>Vous pourrez ensuite l’adopter dans PogHome.</p></div>
      </section>
    </div>
  </div>
</div>
<div class="wrap">
  <header><div class="logo"></div><div><div class="brand">POG Light</div><div class="sub">Light, composed locally</div></div><span class="chip" id="net">Connexion...</span></header>

  <section class="hero">
    <div class="eyebrow">POG Projects · ambient system</div>
    <h1>La lumière,<br>composée.</h1>
    <p class="hero-copy">Façonnez l’atmosphère en temps réel. Chaque réglage reste local, instantané et parfaitement à vous.</p>
    <div class="light-stage"><div class="strip" id="preview"></div></div>
  </section>

  <main class="layout">
    <section class="card mobile-panel mobile-active" id="viewLight">
      <div class="card-head"><div><h2>Atmosphère</h2><div class="kicker">Scène active</div></div></div>
      <div class="grid" id="patterns"></div>
      <div class="scene-controls">
        <label class="color-control"><input type="color" id="primaryColor"><span><span class="control-title">Couleur principale</span><span class="control-value" id="primaryHex">#000000</span></span></label>
        <label class="color-control"><input type="color" id="secondaryColor"><span><span class="control-title">Couleur secondaire</span><span class="control-value" id="secondaryHex">#000000</span></span></label>
        <div class="sliders">
          <div class="row"><label>Luminosité</label><input type="range" id="brightness" min="5" max="255"><span class="val" id="brVal"></span></div>
          <div class="row"><label>Vitesse</label><input type="range" id="speed" min="0" max="100"><span class="val" id="spVal"></span></div>
        </div>
      </div>
    </section>

    <div class="stack">
      <section class="card mobile-panel" id="viewHardware">
        <div class="card-head"><div><h2>Bande LED</h2><div class="kicker">Configuration physique</div></div></div>
        <p class="hint">Utilisez « Ordre couleurs » pour vérifier que rouge, vert et bleu correspondent à votre bande.</p>
        <div class="row"><label>Nombre de LED</label><input type="number" id="numLeds" min="1" max="300"></div>
        <div class="row"><label>Sortie</label><select id="ledPin"></select></div>
        <div class="row"><label>Ordre</label><select id="colorOrder"></select></div>
        <div class="row"><label>Sens</label><select id="reverse"><option value="0">Normal</option><option value="1">Inversé</option></select></div>
        <div class="row"><label>Courant max.</label><input type="number" id="maxMilliAmps" min="100" max="10000" step="100"><span class="val">mA</span></div>
        <div class="row"><label>Mode</label><select id="analog"><option value="0">Adressable · ARGB</option><option value="1">Analogique · PWM</option></select></div>
        <div class="row"><label>Utilité</label><select id="purpose"></select></div>
        <div class="divider"></div>
        <div class="card-head"><div><h2>Périphériques</h2><div class="kicker">Écran & commandes locales</div></div></div>
        <div class="peripheral">
          <div class="peripheral-head"><div><div class="peripheral-title">Écran OLED I²C</div><div class="peripheral-copy">SSD1306 / SSD1315 · 128 × 64</div></div><select id="oledEnabled" onchange="updatePeripheralUi()"><option value="0">Désactivé</option><option value="1">Activé</option></select></div>
          <div class="pin-grid">
            <label class="pin-field">GPIO SDA<select id="oledSda"></select></label>
            <label class="pin-field">GPIO SCL<select id="oledScl"></select></label>
            <label class="pin-field">Adresse<select id="oledAddress"><option value="60">0x3C</option><option value="61">0x3D</option></select></label>
          </div>
          <div class="hint peripheral-status" id="oledStat"></div>
        </div>
        <div class="peripheral">
          <div class="peripheral-head"><div><div class="peripheral-title">Boutons de la lampe</div><div class="peripheral-copy">Haut / bas changent l’effet · gauche / droite ajustent la scène</div></div><select id="buttonsEnabled" onchange="updatePeripheralUi()"><option value="0">Désactivés</option><option value="1">Activés</option></select></div>
          <label class="pin-field">Type d’entrée<select id="buttonMode" onchange="updatePeripheralUi()"><option value="0">Poussoirs · GPIO vers GND</option><option value="1">Touches capacitives</option></select></label>
          <div class="pin-grid">
            <label class="pin-field">Haut<select id="buttonPin0"></select></label>
            <label class="pin-field">Bas<select id="buttonPin1"></select></label>
            <label class="pin-field">Gauche<select id="buttonPin2"></select></label>
            <label class="pin-field">Droite<select id="buttonPin3"></select></label>
          </div>
          <div class="hint peripheral-status" id="buttonStat"></div>
        </div>
        <div class="action-row"><button class="btn" type="button" onclick="applyHw()">Appliquer</button><span class="hint">Redémarre si nécessaire.</span></div>
        <div class="divider"></div>
        <div class="card-head"><div><h2>Sections</h2><div class="kicker">Zones du ruban · PogHome</div></div><button class="btn ghost" type="button" onclick="addSection()">+ Ajouter</button></div>
        <p class="hint">Découpez la bande en lampes indépendantes. Chaque zone apparaît dans PogHome avec son nom et son utilité.</p>
        <div class="section-map" id="sectionMap" aria-label="Carte des sections du ruban"></div>
        <div class="zone-list" id="sectionList"></div>
        <div class="action-row"><button class="btn" type="button" onclick="saveSections()">Enregistrer les sections</button><span class="hint">8 zones maximum.</span></div>
      </section>

      <section class="card mobile-panel" id="viewNetwork">
        <div class="card-head"><div><h2>Connexion</h2><div class="kicker">Réseau & logiciel</div></div></div>
        <div class="row"><label>Réseau Wi-Fi</label><input type="text" id="ssid" placeholder="Nom du réseau"><button class="btn ghost" type="button" onclick="scanWifi()">Rechercher</button></div>
        <div class="network-list" id="wifiList"></div>
        <div class="row"><label>Mot de passe</label><input type="password" id="wifiPass" placeholder="••••••••"></div>
        <div class="action-row"><button class="btn ghost" type="button" onclick="saveWifi()">Connecter</button></div>
        <div class="divider"></div>
        <div class="card-head"><div><h2>Mise à jour</h2><div class="kicker">Firmware local</div></div></div>
        <div class="row"><input type="file" id="ota" accept=".bin"></div>
        <div class="action-row"><button class="btn" type="button" onclick="doOta()">Installer</button><button class="btn ghost" type="button" onclick="reboot()">Redémarrer</button></div>
        <div id="otaWrap" style="display:none"><div class="pbar"><div id="otaBar"></div></div><div class="hint" id="otaPct"></div></div>
      </section>
    </div>
  </main>
</div>
<nav class="mobile-nav" aria-label="Navigation principale">
  <button class="active" type="button" data-view="light" onclick="switchMobileView('light')">
    <svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="4"></circle><path d="M12 2v2M12 20v2M2 12h2M20 12h2M5 5l1.5 1.5M17.5 17.5 19 19M19 5l-1.5 1.5M6.5 17.5 5 19"></path></svg><span>Lumière</span>
  </button>
  <button type="button" data-view="hardware" onclick="switchMobileView('hardware')">
    <svg viewBox="0 0 24 24" aria-hidden="true"><rect x="6" y="5" width="12" height="14" rx="3"></rect><path d="M9 2v3M15 2v3M9 19v3M15 19v3M3 9h3M18 9h3M3 15h3M18 15h3"></path></svg><span>Bande</span>
  </button>
  <button type="button" data-view="network" onclick="switchMobileView('network')">
    <svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4.5 9a11 11 0 0 1 15 0M7.5 12.5a6.5 6.5 0 0 1 9 0M10.5 16a2.3 2.3 0 0 1 3 0"></path><circle cx="12" cy="19" r=".7" fill="currentColor" stroke="none"></circle></svg><span>Réseau</span>
  </button>
</nav>
<div class="toast" id="toast"></div>
<script>
const PATS=["Couleur pleine","Ordre couleurs","Compter (defile)","Remplissage","Arc-en-ciel","Chenillard","Respiration","Feu","Scintillement","Degrade","Balayage","Blanc plein","Eteint"];
const ORDERS=["RGB","RBG","GRB","GBR","BRG","BGR"];
const PURPOSES=["Ambiance","Éclairage de travail","Veilleuse","Balisage","Télévision","Indicateur d’état","Réveil","Décoration"];
let PINS=[18,16,2,15,17,21,38,47,48];
let GPIO_OPTIONS=[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,21,35,36,37,38,39,40,41,42,43,44,45,46,47,48];
let TOUCH_PINS=[1,2,3,4,5,6,7,8,9,10,11,12,13,14];
let st=null;
let sections=[],nextSectionId=1;
function $(i){return document.getElementById(i)}
function api(p,o){return fetch(p,o).then(r=>r.json())}
function toast(m){const t=$('toast');t.textContent=m;t.className='toast show';setTimeout(()=>t.className='toast',1800)}
function switchMobileView(view){
  const ids={light:'viewLight',hardware:'viewHardware',network:'viewNetwork'};
  document.querySelectorAll('.mobile-panel').forEach(x=>x.classList.toggle('mobile-active',x.id===ids[view]));
  document.querySelectorAll('.mobile-nav button').forEach(x=>x.classList.toggle('active',x.dataset.view===view));
  document.body.classList.toggle('mobile-secondary',view!=='light');
  window.scrollTo({top:0,behavior:'smooth'});
}
let obStep=0;
function rememberOnboarding(){try{localStorage.setItem('poglightOnboarded','1')}catch(e){}}
function onboardingDone(){try{return localStorage.getItem('poglightOnboarded')==='1'}catch(e){return false}}
function setObStep(n){
  obStep=n;
  document.querySelectorAll('.ob-page').forEach(x=>x.classList.toggle('on',+x.dataset.step===n));
  document.querySelectorAll('#obProgress i').forEach((x,i)=>x.classList.toggle('on',i===Math.min(n,2)));
  $('obProgress').style.visibility=n===3?'hidden':'visible';
}
function obNext(){setObStep(Math.min(obStep+1,2))}
function obBack(){setObStep(Math.max(obStep-1,0))}
function closeOnboarding(){rememberOnboarding();$('onboarding').classList.remove('show')}
function showOnboarding(c){
  $('obNumLeds').value=c.numLeds||60;
  $('obLedPin').innerHTML=PINS.map(p=>`<option value="${p}"${p==c.ledPin?' selected':''}>GPIO ${p}</option>`).join('');
  $('obAnalog').value=c.analog?'1':'0';
  $('obPurpose').innerHTML=PURPOSES.map((n,i)=>`<option value="${i}"${i==(c.purpose||0)?' selected':''}>${n}</option>`).join('');
  $('obPinHint').textContent='GPIO '+$('obLedPin').value;
  setObStep(0);
  $('onboarding').classList.add('show');
}
async function scanOnboardingWifi(){
  toast('Recherche des réseaux…');
  try{
    const n=await api('/api/scan');
    $('obWifiList').innerHTML=n.sort((a,b)=>b.rssi-a.rssi).slice(0,8).map(x=>`<button class="network" type="button" onclick="$('obSsid').value='${(x.ssid||'').replace(/'/g,'')}';$('obWifiPass').focus()"><span>${x.ssid}</span><span>${x.rssi} dBm</span></button>`).join('');
  }catch(e){toast('Recherche impossible')}
}
async function finishOnboarding(){
  const ssid=$('obSsid').value.trim();
  if(!ssid){toast('Choisissez votre réseau Wi‑Fi');return}
  const button=$('obFinish');button.disabled=true;button.textContent='Connexion…';
  rememberOnboarding();
  try{
    const r=await api('/api/setup',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({
      numLeds:+$('obNumLeds').value,ledPin:+$('obLedPin').value,analog:$('obAnalog').value==='1',
      purpose:+$('obPurpose').value,
      wifiSsid:ssid,wifiPass:$('obWifiPass').value
    })});
    if(!r.ok)throw new Error();
    setObStep(3);
  }catch(e){
    button.disabled=false;button.textContent='Connecter et terminer';
    toast('Connexion interrompue · réessayez');
  }
}
$('obLedPin').addEventListener('change',()=>$('obPinHint').textContent='GPIO '+$('obLedPin').value);

function rgbInt(v){return parseInt(v.slice(1),16)}
function rgbHex(v){return '#'+Number(v||0).toString(16).padStart(6,'0')}
function esc(v){return String(v||'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]))}
function pinOptions(selected){const pins=GPIO_OPTIONS.includes(+selected)?GPIO_OPTIONS:[+selected,...GPIO_OPTIONS];return pins.map(p=>`<option value="${p}"${p==selected?' selected':''}>GPIO ${p}</option>`).join('')}
function liveCfg(){return {pattern:cur.pattern,primaryColor:rgbInt($('primaryColor').value),secondaryColor:rgbInt($('secondaryColor').value),brightness:+$('brightness').value,speed:+$('speed').value,colorOrder:+$('colorOrder').value,reverse:$('reverse').value=='1',maxMilliAmps:+$('maxMilliAmps').value}}
let cur={pattern:4};
function pushLive(){api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(liveCfg())})}

function renderPatterns(){
  $('patterns').innerHTML=PATS.map((n,i)=>`<button class="pat${i==cur.pattern?' on':''}" onclick="selPat(${i})">${n}</button>`).join('');
}
function selPat(i){cur.pattern=i;renderPatterns();pushLive()}

function fill(c){
  cur.pattern=c.pattern;
  $('primaryColor').value=rgbHex(c.primaryColor);
  $('secondaryColor').value=rgbHex(c.secondaryColor);
  $('primaryHex').textContent=$('primaryColor').value;
  $('secondaryHex').textContent=$('secondaryColor').value;
  $('brightness').value=c.brightness;$('brVal').textContent=Math.round(c.brightness/255*100)+'%';
  $('speed').value=c.speed;$('spVal').textContent=c.speed+'%';
  $('numLeds').value=c.numLeds;
  $('ledPin').innerHTML=PINS.map(p=>`<option value="${p}"${p==c.ledPin?' selected':''}>GPIO ${p}</option>`).join('');
  $('colorOrder').innerHTML=ORDERS.map((n,i)=>`<option value="${i}"${i==c.colorOrder?' selected':''}>${n}</option>`).join('');
  $('analog').value=c.analog?'1':'0';
  $('purpose').innerHTML=PURPOSES.map((n,i)=>`<option value="${i}"${i==(c.purpose||0)?' selected':''}>${n}</option>`).join('');
  $('reverse').value=c.reverse?'1':'0';
  $('maxMilliAmps').value=c.maxMilliAmps;
  $('oledEnabled').value=c.oledEnabled?'1':'0';
  $('oledSda').innerHTML=pinOptions(c.oledSda??13);
  $('oledScl').innerHTML=pinOptions(c.oledScl??11);
  $('oledAddress').value=String(c.oledAddress||60);
  $('buttonsEnabled').value=c.buttonsEnabled?'1':'0';
  $('buttonMode').value=String(c.buttonMode||0);
  const bp=c.buttonPins||[5,6,7,4];
  bp.forEach((pin,i)=>$('buttonPin'+i).innerHTML=pinOptions(pin));
  updatePeripheralUi();
  $('ssid').value=c.wifiSsid||'';
  sections=(c.sections||[]).map(x=>({...x}));
  nextSectionId=c.nextSectionId||Math.max(1,...sections.map(x=>x.id+1));
  renderSections();
  renderPatterns();
}
$('brightness').addEventListener('change',pushLive);
$('brightness').addEventListener('input',()=>$('brVal').textContent=Math.round(+$('brightness').value/255*100)+'%');
$('speed').addEventListener('change',pushLive);
$('speed').addEventListener('input',()=>$('spVal').textContent=$('speed').value+'%');
$('colorOrder').addEventListener('change',pushLive);
$('primaryColor').addEventListener('input',()=>{$('primaryHex').textContent=$('primaryColor').value;pushLive()});
$('secondaryColor').addEventListener('input',()=>{$('secondaryHex').textContent=$('secondaryColor').value;pushLive()});
$('reverse').addEventListener('change',pushLive);
$('maxMilliAmps').addEventListener('change',pushLive);

async function applyHw(){
  const oledOn=$('oledEnabled').value==='1',buttonsOn=$('buttonsEnabled').value==='1';
  const oledPins=[+$('oledSda').value,+$('oledScl').value],buttonPins=[0,1,2,3].map(i=>+$('buttonPin'+i).value);
  if(oledOn&&oledPins[0]===oledPins[1]){toast('SDA et SCL doivent utiliser deux GPIO différents');return}
  if(buttonsOn&&new Set(buttonPins).size!==4){toast('Chaque bouton doit avoir son propre GPIO');return}
  if(buttonsOn&&+$('buttonMode').value===1&&buttonPins.some(pin=>!TOUCH_PINS.includes(pin))){toast('Une broche choisie n’est pas compatible tactile');return}
  const used=[{pin:+$('ledPin').value,name:'sortie LED'}];
  if(oledOn)used.push({pin:oledPins[0],name:'OLED SDA'},{pin:oledPins[1],name:'OLED SCL'});
  if(buttonsOn)buttonPins.forEach((pin,i)=>used.push({pin,name:['bouton haut','bouton bas','bouton gauche','bouton droite'][i]}));
  const conflict=used.find((x,i)=>used.some((y,j)=>j<i&&y.pin===x.pin));
  if(conflict){toast('GPIO '+conflict.pin+' déjà utilisé · choisissez une autre broche');return}
  const cfg={numLeds:+$('numLeds').value,ledPin:+$('ledPin').value,analog:$('analog').value=='1',purpose:+$('purpose').value,reverse:$('reverse').value=='1',maxMilliAmps:+$('maxMilliAmps').value,
    oledEnabled:oledOn,oledSda:oledPins[0],oledScl:oledPins[1],oledAddress:+$('oledAddress').value,
    buttonsEnabled:buttonsOn,buttonMode:+$('buttonMode').value,buttonPins};
  const r=await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(cfg)});
  if(r.reboot){toast('Redémarrage en cours…');setTimeout(()=>location.reload(),8000);}else toast('Réglages appliqués');
}
function updatePeripheralUi(){
  const oledOn=$('oledEnabled').value==='1',buttonsOn=$('buttonsEnabled').value==='1';
  ['oledSda','oledScl','oledAddress'].forEach(id=>$(id).disabled=!oledOn);
  ['buttonMode','buttonPin0','buttonPin1','buttonPin2','buttonPin3'].forEach(id=>$(id).disabled=!buttonsOn);
  if(st&&!st.touchSupported&&+$('buttonMode').value===1)$('buttonMode').value='0';
  const touchOption=$('buttonMode').querySelector('option[value="1"]');
  if(touchOption)touchOption.disabled=!!st&&!st.touchSupported;
  $('buttonStat').textContent=!buttonsOn?'Commandes locales désactivées':(+$('buttonMode').value===0?'Branchez chaque poussoir entre son GPIO et GND.':'Utilisez une électrode tactile par GPIO.');
}
function sectionById(id){return sections.find(x=>x.id===id)}
function setSection(id,key,value){
  const s=sectionById(id);if(!s)return;
  s[key]=value;
  if(key==='start'||key==='count'||key==='enabled'||key==='primaryColor'||key==='on')renderSectionMap();
}
function renderSectionMap(){
  const total=Math.max(1,+$('numLeds').value||60);
  $('sectionMap').innerHTML=sections.map(s=>{
    const left=Math.min(100,Math.max(0,s.start/total*100));
    const width=Math.min(100-left,Math.max(.7,s.count/total*100));
    return `<i class="section-piece${(!s.enabled||!s.on)?' off':''}" style="left:${left}%;width:${width}%;--zone:${rgbHex(s.primaryColor)}" title="${esc(s.name)} · LED ${s.start+1}–${s.start+s.count}"></i>`;
  }).join('');
}
function renderSections(){
  renderSectionMap();
  if(!sections.length){$('sectionList').innerHTML='<div class="empty-zones">Aucune section · toute la bande reste pilotée comme une seule lumière.</div>';return}
  $('sectionList').innerHTML=sections.map(s=>`<article class="zone" style="--zone:${rgbHex(s.primaryColor)}">
    <div class="zone-head"><i class="zone-dot"></i><input aria-label="Nom de la section" value="${esc(s.name)}" oninput="setSection(${s.id},'name',this.value)"><button class="icon-btn" type="button" aria-label="Supprimer ${esc(s.name)}" onclick="removeSection(${s.id})">×</button></div>
    <div class="zone-grid">
      <label class="zone-field">Première LED<input type="number" min="1" max="${+$('numLeds').value}" value="${s.start+1}" onchange="setSection(${s.id},'start',Math.max(0,+this.value-1))"></label>
      <label class="zone-field">Dernière LED<input type="number" min="1" max="${+$('numLeds').value}" value="${s.start+s.count}" onchange="setSection(${s.id},'count',Math.max(1,+this.value-${s.start}))"></label>
      <label class="zone-field wide">Utilité<select onchange="setSection(${s.id},'purpose',+this.value)">${PURPOSES.map((n,i)=>`<option value="${i}"${i===s.purpose?' selected':''}>${n}</option>`).join('')}</select></label>
      <label class="zone-field">Effet<select onchange="setSection(${s.id},'pattern',+this.value)">${PATS.map((n,i)=>`<option value="${i}"${i===s.pattern?' selected':''}>${n}</option>`).join('')}</select></label>
      <label class="zone-field">Couleur<input class="zone-color" type="color" value="${rgbHex(s.primaryColor)}" onchange="setSection(${s.id},'primaryColor',rgbInt(this.value));renderSections()"></label>
      <label class="zone-field">Couleur secondaire<input class="zone-color" type="color" value="${rgbHex(s.secondaryColor)}" onchange="setSection(${s.id},'secondaryColor',rgbInt(this.value))"></label>
      <label class="zone-field">Vitesse<input type="range" min="0" max="100" value="${s.speed}" oninput="setSection(${s.id},'speed',+this.value)"></label>
      <label class="zone-field">Luminosité<input type="range" min="0" max="255" value="${s.brightness}" oninput="setSection(${s.id},'brightness',+this.value);this.closest('.zone').querySelector('.range-value').textContent=Math.round(this.value/255*100)+'%'"></label>
    </div>
    <div class="zone-foot"><label><input type="checkbox"${s.enabled?' checked':''} onchange="setSection(${s.id},'enabled',this.checked)"> Active</label><label><input type="checkbox"${s.on?' checked':''} onchange="setSection(${s.id},'on',this.checked)"> Allumée</label><span class="range-value">${Math.round(s.brightness/255*100)}%</span></div>
  </article>`).join('');
}
function addSection(){
  if(sections.length>=8){toast('8 sections maximum');return}
  const total=Math.max(1,+$('numLeds').value||60);
  const used=sections.reduce((m,s)=>Math.max(m,s.start+s.count),0);
  const start=Math.min(used,total-1);
  sections.push({id:nextSectionId++,name:'Section '+(sections.length+1),start,count:Math.max(1,total-start),enabled:true,on:true,purpose:+$('purpose').value||0,brightness:255,pattern:0,primaryColor:rgbInt($('primaryColor').value),secondaryColor:rgbInt($('secondaryColor').value),speed:+$('speed').value});
  renderSections();
}
function removeSection(id){sections=sections.filter(x=>x.id!==id);renderSections()}
async function saveSections(){
  const total=Math.max(1,+$('numLeds').value||60);
  sections.forEach(s=>{s.start=Math.min(total-1,Math.max(0,s.start));s.count=Math.min(total-s.start,Math.max(1,s.count));s.name=(s.name||'Section').trim().slice(0,32)});
  const r=await api('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({purpose:+$('purpose').value,nextSectionId,sections})});
  if(r.ok){renderSections();toast('Sections publiées dans PogHome')}else toast('Enregistrement impossible');
}
async function scanWifi(){toast('Recherche des réseaux…');try{const n=await api('/api/scan');$('wifiList').innerHTML=n.sort((a,b)=>b.rssi-a.rssi).slice(0,12).map(x=>`<button class="network" onclick="$('ssid').value='${(x.ssid||'').replace(/'/g,'')}';$('wifiPass').focus()"><span>${x.ssid}</span><span>${x.rssi} dBm</span></button>`).join('');}catch(e){toast('Recherche impossible')}}
async function saveWifi(){const s=$('ssid').value.trim();if(!s){toast('Choisissez un réseau');return}toast('Connexion et redémarrage…');try{await api('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({wifiSsid:s,wifiPass:$('wifiPass').value})})}catch(e){}}
async function reboot(){if(!confirm('Redémarrer PogLight ?'))return;try{await api('/api/reboot',{method:'POST'})}catch(e){}toast('Redémarrage en cours…')}
function doOta(){const f=$('ota').files[0];if(!f){toast('Choisissez un firmware .bin');return}const fd=new FormData();fd.append('f',f);const x=new XMLHttpRequest();x.open('POST','/api/ota');$('otaWrap').style.display='block';
  x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded/e.total*100);$('otaBar').style.width=p+'%';$('otaPct').textContent='Envoi '+p+'%'}};
  x.onload=()=>{let r={};try{r=JSON.parse(x.responseText)}catch(e){}if(r.ok){$('otaPct').textContent='Installé · redémarrage…';setTimeout(()=>location.reload(),8000)}else toast('La mise à jour a échoué')};
  x.onerror=()=>toast('Connexion interrompue');x.send(fd)}

async function pollPreview(){try{const h=await (await fetch('/api/leds')).text();const s=$('preview');const N=Math.floor(h.length/6);let o='';for(let i=0;i<N;i++){const c='#'+h.substr(i*6,6);o+= (c!='#000000')?`<i style="background:${c};box-shadow:0 0 7px ${c}"></i>`:'<i></i>'}s.innerHTML=o}catch(e){}}
function netInfo(s){
  $('net').textContent=s.apMode?'AP : 192.168.4.1':(s.ip||'-');
  const o=$('oledStat');
  const c=s.config||{};
  if(o) o.innerHTML = !c.oledEnabled
    ? 'Écran désactivé'
    : s.oledFound
      ? ('Écran OLED · <span style="color:var(--emerald)">détecté</span> · 0x'+(s.oledAddr||0).toString(16)+' · SDA '+c.oledSda+' / SCL '+c.oledScl)
      : '<span style="color:var(--danger)">Écran OLED non détecté</span> · vérifiez les GPIO, VCC et GND';
}
async function load(){st=await api('/api/state');if(st.board=='esp32c3'){PINS=[2,3,4,5,6,7,10];GPIO_OPTIONS=[0,1,2,3,4,5,6,7,8,9,10,20,21];TOUCH_PINS=[]}else if(st.board=='esp32'){PINS=[2,4,5,12,13,14,16,17,18,19,21,22,23];GPIO_OPTIONS=[2,4,5,12,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33];TOUCH_PINS=[4,2,15,13,12,14,27,33,32]}fill(st.config);netInfo(st);const preview=new URLSearchParams(location.search).has('onboarding');if(preview||(st.apMode&&!onboardingDone()))showOnboarding(st.config)}
load();
setInterval(pollPreview,500);
setInterval(async()=>{try{netInfo(await api('/api/state'))}catch(e){}},5000);
</script></body></html>)HTML";
