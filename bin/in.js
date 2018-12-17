//var dllt = bb.ld('I:/ws/hjs/release/dllt.dll')
//var dllt = bb.ld('./dllt')
//var dllt = bb.ld('I:/ws/hjs/release/dllt.dll')
//bb.lg(dllt.x,dllt.y);

cc = ui.getXY();
bb.lg()
bb.lg('cc --> ',cc.x,cc.y,cc.h,cc.w);

//bb.lg(null)

var j=0;
for(j=0; j < ARGV.length; ++j) {
    bb.lg(ARGV[j]);
}

//iii = bb.inc('./inc2.js')


sc = bb.listen(40001)
scs = [];
sc_islisten = true;
if( sc === false) {
    bb.lg('Listen FALSE')
    sc_islisten = false;
    sc = bb.connect('localhost:40001')
    if( sc === false) {
        bb.lg('connect FALSE')
    }else bb.lg('connect OK')
}else{
    bb.lg('Listen OK')
}

sh = bb.buf(-1000,'77');
//sh = bb.buf(10000);

function exit_w(foo,id)
{
    ui.close();
}

function ff(foo,id)
{
    io.log('ff> foo: ',foo," id: ",id,"\n");
}

function tmr(foo,id)
{
/*
    io.log('tmr> foo: ',foo," id: ",id,"\n");
    var in_s = bb.rd();
    if( in_s) {
        bb.lg(in_s);
    }
*/
    if( sc_islisten ) {
        var nsc = sc.select();
        if( nsc !== false) {
            scs.push(nsc);
        }
        var i=0;
        for( i = 0; i < scs.length; ++i) {
            if( scs[i].select()) {
                ui.add(txt,scs[i].rd())
            }
        }
    } else {
        if( sc.select()) {
            ui.add(txt,sc.rd())
        }
    }
}

dlg = ui.dlg();
function dlg_close(foo,id){ui.close(dlg);}
a5 = ui.action("Закрыть диалог",function(foo,id){ui.close(dlg);},"img/cut.png");

ui.def(dlg);
ui.sep('Новое слово');
ui.button("Закрыть диалог.",dlg_close,"img/back.gif");
ui.button("Ура всем!!!",ff,"img/back.gif");
ui.sep('Новое слово');
ui.space()
ui.def();

function dlg_f(foo,id)
{
//    io.log('tmr> foo: ',foo," id: ",id,"\n");
    var p = ui.getXY();
    io.log('dlg_f> x: ',p.x," y: ",p.y,"\n");

    ui.show('Диалог',0,0,p.x,p.y,dlg);
}

function mes_dlg_f(foo,id)
{
//    io.log('mes_dlg_f> foo: ',foo," id: ",id," RET ",ui.mes(2,'Вопрос','Ответы??'),"\n");
    io.log('mes_dlg_f> foo: ',foo," id: ",id," RET ",ui.file(1,'Вопрос','./img',"Images (*.png *.xpm *.jpg);;Text files (*.txt);;XML files (*.xml)"));
}



a1 = ui.action("Закрыть процесс",function(){
prc.close()
},"img/open.png");
a2 = ui.action("Файл расстрелять",ff,"img/paste.xpm");
a3 = ui.action("Файл оприходовать",ff,"img/back.gif");
a4 = ui.action("Перемотать",function(){vc.read(0)},"img/cut.png");

a99_99 = ui.action("EXIT",exit_w,"img/back.gif");

ui.menus('Первый');
ui.menu(a1);
ui.menu(a2);
ui.menu_sep();
ui.menu(a99_99);

ui.menus('Второй');
ui.menus('еще один');
ui.menus('и Потом');
ui.menu(a1);
ui.menu_sep();
ui.menu(a2);
ui.menu_sep();
ui.menu(a3);
ui.menu(a4);

ui.tool(a3);
ui.tool(a1);
ui.tool(a2);

ui.tools();
ui.tool(a1);
ui.tool_sep();
ui.tool(a2);
ui.tool(a4);
ui.tool(a5);

//--

/*

ui.lcd(4,4567);
gui.button("Ура всем!!!",0,"img/back.gif");
gui.button("Ура всем!!!");
gui.space();
h = gui.hbox();
gui.def(h);
gui.button("Ура всем!!!");
gui.space();
gui.button("Ура всем!!!");
gui.button("Ура всем!!!");

gui.def();

gui.button("Ура всем!!!");
ui.button("Так возможно?",0,"img/arrow.xpm");




ui.check("Ура всем!!!","img/ZombieHand2.gif");
ui.sep();
h1 = ui.hbox();
ui.def(h1);
ui.space();
ui.sep();
ui.button("Сохраняем",0,"img/save.png");
ui.def();

g = ui.grid(3);
ui.def(g);
gui.button("Ура всем!!!",0,"img/back.gif");
gui.button("Ура всем!!!",0,"img/back.gif");
gui.button("Ура всем!!!",0,"img/back.gif");
gui.button("Ура всем!!!",0,"img/back.gif");
gui.button("Ура всем!!!",0,"img/back.gif");


ui.radio("Ура всем!!!","img/ZombieHand2.gif");
ui.radio("Ура всем!!!","img/ZombieHand2.gif");
ui.radio("Ура всем!!!","img/ZombieHand2.gif");
ui.radio("Ура всем!!!","img/ZombieHand2.gif");
ui.def();
ui.sep();
bg = ui.buttons();
ui.radio("Ура всем!!!","img/ZombieHand2.gif");
ui.radio("Ура всем!!!","img/ZombieHand2.gif");
ui.sep();
g2 = ui.grid();
ui.def(g2);
ui.lab('Метка черная');
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);
ui.lab('Метка черная');
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);
ui.lab('Метка черная');
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);
ui.lab('Метка черная');
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);
ui.lab('Метка черная');
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);

ui.lab('Метка черная','img/paste.png');
gui.button("Ура всем!!!",0,"img/back.gif");

ui.def();
gbb = ui.hbox();
ui.def(gbb);
ui.lab('Метка черная','img/paste.png');
ui.lab('Метка черная');
gui.button("Ура всем!!!",0,"img/back.gif");
ui.space();
ui.lcd(4,4567);

ui.def()
ui.prog(35)
*/

/*
ui.combo()
ui.line()
ui.lab("Вводим текст:")
ui.text()
ui.ptext()
ui.spin(45,2,3);
ui.spin();
hh = ui.hbox()
ui.def(hh)
ui.dial(45,2,3);
ui.dial(45,2,3);
ui.dial(45,2,3);

ui.dttm();

ui.space()
ui.hslider()
ui.def()
ui.vslider()
*/

t = ui.tab();
tl1 = ui.add(t,'Нечто','img/back.gif');
ui.def(tl1);
dial1 = ui.dial(45,2,3);
ui.set(dial1,20);
bg = ui.buttons();
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);
rb = ui.radio("!!!Ура всем!!!","img/ZombieHand2.gif",bg);
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);
ui.radio("Ура всем!!!","img/ZombieHand2.gif",bg);ui.space();
ui.set(bg,3);
//ui.set(rb);
ch = ui.check("чекер","img/ZombieHand2.gif");
ui.set(ch,2);
dt = ui.dttm();
ui.set(dt,2012,11,15)
d1 = ui.get(dt);
io.log('dt ** ',d1.y,' ',d1.m,' ',d1.d,' ',"\n");

tl2 = ui.add(t,'Ужас','img/arrow_down.xpm');
ui.def(tl2);
gui.button("Выйти!",exit_w,"img/back.gif");
gui.button("Ура всем!!!",function() {
    var dd;
    if( !sc_islisten) {
        dd = ui.get(cb)
        bb.lg(dd)
        sc.wr(dd)
    } else {
        dd = ui.get(cb)
        var i=0;
        for( i = 0; i < scs.length; ++i) {
            scs[i].wr(dd)
        }
    }
}
,"img/back.gif");
ui.space();
h1 = ui.hbox();
ui.def(h1);
ui.space();
ui.sep();
ui.button("Сохраняем",mes_dlg_f,"img/save.png");

tl3 = ui.add(t,'Исчо','img/delete.xpm');
ui.def(tl3);
ui.sep('Сепаратор');

h2 = ui.hbox();
ui.def(h2);
ui.space();
ui.sep();
ui.button("",dlg_f,"img/save.png",'Открыть диалог - подсказка',999);

ui.button("Установить Шаред",function(){
    sh.st(ui.get(cb));
    //ui.set(l_sh,ui.get(cb));
},"",'подсказка',979);

ui.button("Прочитать Шаред",function(){
    //sh.st(ui.get(cb));
    ui.set(l_sh,sh.gt());
},"",'подсказка',978);

ui.def(tl3);
ui.space()
ui.sep('Другая тема - Комбо');
l_sh = ui.lab('Шаред лабел');
cb = ui.combo()
ui.add(cb,'таблица1','img/save.png');
ui.add(cb,'таблица2','img/grid.xpm');
ui.add(cb,'таблица3','img/grid.xpm');
ui.add(cb,'таблица4');
ui.add(cb,'таблица5','img/grid.xpm');
ui.add(cb,'таблица6','img/save.png');
ui.add(cb,'таблица7','img/grid.xpm');
ui.set(cb,'Установили');

io.log('combo ** ',"\n");
io.log('combo ** ',ui.get(cb),"\n");

tl4 = ui.add(t,'таблица','img/grid.xpm');
ui.def(tl4);
tb = ui.table(ff,777,[['первая',150,'img/grid.xpm'],['2',200,'img/back.gif'],['третья',100,'img/save.png',777]],[a2,a4]);
ui.add(tb,[['Нечто','img/back.gif'],['двойка','img/save.png'],['третья','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],['третья','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],['двойка','img/save.png'],['третья','img/_save.png']]);
ui.add(tb,[['Ну','img/back.gif'],['двойка','img/save.png',1],['третья','img/save.png']]);
ui.add(tb,[['Эу?','img/_back.gif'],['Спокойно!','img/save.png',2],['третья','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png',0]]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);
ui.add(tb,[['Нечто','img/back.gif'],[],['двойка','img/save.png']]);

ui.pos(tb,13);
io.log('tb> : ',ui.pos(tb),"\n");

tl5 = ui.add(t,'Дерево','img/KeysOnChain.xpm');
ui.def(tl5);
tree = ui.tree(ff,477,[['первая',150,'img/grid.xpm'],['22',200,'img/back.gif'],['третья',100,'img/save.png',777]],[a1,a2,a4,a3]);
tr1 = ui.add(tree,[['Нечто','img/back.gif'],['двойка','img/save.png',2],['двойка','img/save.png']]);
ui.add(tree,[['Нечто2','img/back.gif'],['двойка','img/save.png',2],['двойка','img/save.png']],tr1);
ui.add(tree,[['Нечто3','img/back.gif',2],['двойка','img/save.png',0],['двойка','img/save.png',1]],tr1);
ui.add(tree,[['Нечто4','img/back.gif'],['двойка','img/save.png',2],['двойка','img/save.png',0]],tr1);
ui.add(tree,[['Нечто5','img/back.gif'],['двойка','img/save.png',0],['двойка','img/save.png']],tr1);
ui.add(tree,[['Нечто6','img/back.gif'],['двойка','img/save.png',2],['двойка','img/save.png']],tr1);
ui.add(tree,[['Top','img/back.gif'],['2','img/save.png',2],['3','img/save.png']]);

ui.pos(tree,5);
io.log('tree> : ',ui.pos(tree),"\n");
//ui.set(tree,6,[['Новое','img/back.gif'],['Н2','img/save.png',1],['Совершенно','img/save.png']]);


a999 = ui.action("tree test",function(foo,id){
    ui.set(tree,8,[['Нечто6','img/back.gif'],['двойка','img/save.png',2],['двойка','img/save.png']]);
    io.log('tree test> foo: ',foo," id: ",id,"\n");
    },"img/polygon.xpm");

ui.tool(a999);

tl6 = ui.add(t,'Список','img/copy.png');
ui.def(tl6);
ui.sep('Список невероятный')
lis = ui.list(ff,352,[a1,a2,a4,a3]);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',2);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',0);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',0);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/back.gif',1);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'знаем','img/cut.xpm',0);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm');
ui.add(lis,'Нечто Лист','img/arrow_up.xpm');
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/back.gif',1);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',2);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',0);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',0);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/back.gif',1);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',2);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',0);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',0);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/back.gif',1);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',2);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',0);
ui.add(lis,'Нечто Лист','img/arrow_up.xpm',0);
ui.add(lis,'Нечто Лист','img/back.gif',2);
ui.add(lis,'Нечто Лист','img/back.gif',1);
ui.set(lis,3,"Установленное значение",'img/map_info.xpm',2)

ui.pos(lis,19);
lval = ui.get(lis,7);
io.log('lis> : ',ui.pos(lis)," ",lval.v,lval.c,"\n");
tv= ui.get(tb,4);
io.log('tb 3> :', tv[0].v,' ',tv[0].c,' ', tv[1].v,' ',tv[1].c,' ', tv[2].v,' ',tv[2].c,"\n");
trv= ui.get(tree,4);
io.log('trv > :', trv[0].v,' ',trv[0].c,' ', trv[1].v,' ',trv[1].c,' ', trv[2].v,' ',trv[2].c,"\n");

//ui.del(lis,2);
io.log('lis> : ',ui.pos(lis),"\n");
//ui.del(lis,1);
//ui.del(lis,1);
//ui.del(tb,4);
//ui.del(tree,4);
io.log('tree> : ',ui.pos(tree),"\n");

ui.timer(tmr,50,666);



io.log('Журнал !!!!\n');
ff(1,2);
bb.lg('Журнал !!!!\n');

a99 = ui.action("Очистить",function(foo,id){
    ui.clr(lis);
    ui.clr(tree);
    ui.clr(tb);
    ui.clr(cb);

    },"img/delete.xpm");
ui.tool(a99);

/*
tl7 = ui.add(t,'WEB','img/arrow.xpm');
//ui.def();
ui.def(tl7);
ui.sep('Браузер')
wb = ui.web();
wb.ld('http:://www.lenta.ru');
//wb.ld('http://89.175.185.217/');
//wb.ld('http://www.youtube.com/watch?v=RNcjWacq7R8')
*/

bb.lg('Журнал 20\n');

bb1 = bb.buf(100000);
bb2 = bb1.slice(500,1000);

bb.lg('Журнал 21\n');

bb2.st('Странности всякие и не то даже.');
//bb.lg('>>>',bb2.gt(),8,99);

bb.lg('Журнал 21\n');

bz0 = bb.buf('Ужас бред Странности всякие и не то даже.Ужас бред Странности всякие и не то даже.Ужас бред Странности всякие и не то даже.Ужас бред Странности всякие и не то даже.Ужас бред Странности всякие и не то даже.Ужас бред Странности всякие и не то даже.Ужас бред Странности всякие и не то даже.Ужас бред Странности всякие и не то даже.');
bzz = bz0.z();
bzzu = bzz.uz();

bb.lg('Журнал 22\n');

//bb.lg('>>>',bzzu.gt());
//bb.lg('>>>',bz0.gt());
//bb.lg('===',bb.rd('./bb.h'))

tl8 = ui.add(t,'TEXT','img/map_info.xpm');
ui.def(tl8);
ui.sep('Текст')
txt = ui.txt();
var sld = ui.hslider()


gamma = 'gamma'
iii = 1
ui.set(txt,'Некий текст')
//ui.set(txt,bb.rd('./bb.h',0).gt());
ui.set(txt,gamma + iii)
ui.add(txt,gamma + iii + '!!!')

bb.lg('Журнал 30\n');

//bb.wr('Пишется некий текст на консоль','./out.hjs')
//bb.wr(bzzu,'./out.hjs')
//bb.wr(bzzu,'./o.hjs')


/*
var urls = bb.curl('http://www.lenta.ru');
wb.set(urls.ctx.gt(0,'windows-1251'));
var u8txt = urls.ctx.gt(0,'windows-1251');
ui.set(txt,urls.hd.gt(0,'windows-1251'));
bb.wr(urls.ctx.gt(0,'windows-1251'),'./ctx.htm')
*/

/*
bb.db('QPSQL','','localhost','web_search','ws_us','websearch')
sq = bb.sql('','select * from slovary')
sq.exec()
var r;
var ii = 0;
while( r = sq.next() ) {
//var uzsl = r.slovar.uz(10000000);
bb.lg(++ii,r.tid,r.stype,r.nm,r.slovar.uz(10000000).gt());
//bb.lg(++ii,r.tid,r.stype,r.nm);

}
*/

//prc = bb.proc('./hjs in_w.js')

//var cvw = bb.ld('./cvw')

//var cvw = bb.ld('../cvw/cvw')

//var cvw = bb.ld('I:/ws/hjs/release/cvw.dll')
//var cvw = bb.ld('cvw.dll')

//bb.lg('load >>> ',cvw,cvw.y)

var vlc = bb.ld('./vlcjs')
bb.lg('load vlc>>> ',vlc,vlc.y)

bb.lg('ShOW')

ui.set(t,7);
gui.show('Нечто невероятноe - волшебство',800,400,100,100);

bb.lg('ShOW 1')


//var mat = cvw.imread('./img/starry_night.jpg')

//bb.lg('ShOW 1.2')
//cvw.namedWindow('Мое',100,620)
//bb.lg('ShOW 1.3')
//var mat2 = mat.clone()
//bb.lg('ShOW 1')
//cvw.imshow(mat2,'Мое')
//bb.lg('ShOW 1.4')
//cvw.namedWindow('Видео',910,100)
//bb.lg('ShOW 1.5')
//cvw.namedWindow('Серое Видео',1640,100)

//bb.lg('ShOW 2')

//vc = cvw.VC('./Video/St.mp4')
//bb.lg('cvw',vc.w,vc.h,vc.fps,vc.fs);
//var mmm = cvw.mat();
//var mmm1 = cvw.mat();
//var m2 = cvw.mat();

var nn=0
var last_frm = 0;
var last_pos = 0;

bb.lg('ShOW 3')

//ui.timer(function(){
//bb.lg('0');
//    if( ui.get(sld) != last_frm) {
//        vc.read(ui.get(sld)*vc.fs/100.);
//        last_pos = ui.get(sld)*vc.fs/100.;
//        last_frm = ui.get(sld);

//        return;
//    }

//    if( last_pos >= (vc.fs))  return;
    
////    return;

////	var mmm = cvw.mat();
//    var r = last_pos = vc.read(mmm);
    
////    cvw.imshow(mmm,'Видео');
////    cvw.imshow(mmm,'Серое Видео')
    
////    return;
    
    
//    if( r !== false && r < vc.fs) {
////        cvw.sobel(mmm,mmm);
////        cvw.blur(mmm,mmm,5,5)
////        cvw.sobel(mmm,mmm);

//        ui.set(sld,(r /vc.fs)*100 );
//        last_frm = ui.get(sld);
        

/////*
//bb.lg('1 ',r,vc.fs);


//        cvw.cvtColor(mmm,mmm1);
//bb.lg('2');
//        cvw.canny(mmm1,m2,200,255);

//bb.lg('3');

//        var cc = cvw.contours(m2);
//bb.lg('3.1');
//        var i;
//        for(i=0; i < cc.length; ++i) {
//            cvw.rectangle(mmm,cc[i],{x:100,y:100},0xff,2,9,0);
//        }
//bb.lg('4');


////        cvw.circle(mmm,{x:100,y:100},70,0xff00,2,9,0);
////        cvw.line(mmm,{x:10,y:10},{x:100,y:100},0xffff,2,9,0);
////        cvw.rectangle(mmm,{x:10,y:10,w:100,h:175},{x:100,y:100},0xff00ff,2,9,0);

////        cvw.threshold(mmm1,mmm1)

////        cvw.sobel(mmm,mmm,-1,2,2,3);
////        cvw.hist(mmm1,mmm);

////        cvw.canny(mmm,mmm1,200,255);
////*/
//        cvw.imshow(mmm,'Видео')
//        cvw.imshow(m2,'Серое Видео')
        
////        cvw.imshow(mmm1,'Серое Видео')

//bb.lg('5');
//    }
////    bb.lg(++nn);
//},1000/vc.fps,66);

/*
bb.rm('./o.hjs')
bb.mkdir('./hjs_add')
bb.rm('./hjs_add')
*/

/*
var dl = bb.dir('./')
//var dl = bb.dir()
bb.lg('dl length ',dl.length);
for(var i=0; i < dl.length; ++i) {
bb.lg(dl[i].filepath,dl[i].isDir);
}
*/

//ui.show('Диалог',0,0,0,0,dlg);
//gui.show();
