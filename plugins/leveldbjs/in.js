
if( ARGV[0] == 'ej')
	levdb = bb.ld('liblevw.so')
else
	levdb = bb.ld('levw')

t_db = levdb.open('tst')


t_db.close()
