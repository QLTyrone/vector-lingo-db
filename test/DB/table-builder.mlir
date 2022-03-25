 // RUN: db-run %s %S/../../resources/data/test | FileCheck %s
 !test_table_tuple=type tuple<!db.nullable<!db.string>,!db.nullable<i1>,!db.nullable<i32>,!db.nullable<i64>,!db.nullable<f32>,!db.nullable<f64>,!db.nullable<!db.date<day>>,!db.nullable<!db.date<millisecond>>,!db.nullable<!db.decimal<5,2>>>
 module {
    //CHECK: |                          str!  |                         bool!  |                        int32!  |                        int64!  |                      float32!  |                      float64!  |                       date32!  |                       date64!  |                      decimal!  |
    //CHECK: ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    //CHECK: |                         "str"  |                          true  |                   -1717986918  |                           110  |                         1e-45  |                        5e-324  |                    1970-01-02  |                    1970-01-01  |                 8205408000.00  |
    //CHECK: |                          null  |                          null  |                          null  |                          null  |                          null  |                          null  |                          null  |                          null  |                          null  |
	func @main () -> !db.table {
	    %str_const = db.constant ( "---------------" ) :!db.string
        %table_builder=db.create_table_builder ["str!","bool!","int32!","int64!","float32!","float64!","date32!","date64!","decimal!"] : !db.table_builder<!test_table_tuple>
        %0 = db.scan_source "{ \"table\": \"test\", \"columns\": [\"str\",\"float32\",\"float64\",\"decimal\",\"int32\",\"int64\",\"bool\",\"date32\",\"date64\"] }" : !db.iterable<!db.iterable<!test_table_tuple,table_row_iterator>,table_chunk_iterator>
        db.for %table_chunk in %0 : !db.iterable<!db.iterable<!test_table_tuple,table_row_iterator>,table_chunk_iterator> {
            db.for %table_row in %table_chunk : !db.iterable<!test_table_tuple,table_row_iterator> {
                db.add_row %table_builder : !db.table_builder<!test_table_tuple>, %table_row : !test_table_tuple
                db.yield
            }
            db.yield
        }
        %table_mat=db.finalize_table %table_builder : !db.table_builder<!test_table_tuple>
		return %table_mat : !db.table
	}
 }