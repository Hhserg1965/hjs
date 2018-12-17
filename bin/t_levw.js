bb.lg(ARGV)
if( ARGV[0].indexOf('hjs') >=0 ){
    lv = bb.ld('levw')
}else{
    lv = bb.ld('./liblevw.so')
}
bb.lg(lv)
t_db = lv.open('tdb')

t_db.set('ttt','OK_RESULT')

bb.lg(t_db.get('ttt'))

t_db.close()
