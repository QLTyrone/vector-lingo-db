module {
  func.func @main() {
    %0 = relalg.basetable  {table_identifier = "items"} columns: {embedding => @items::@embedding({type = !db.nullable<!db.vector<3>>}), id => @items::@id({type = !db.nullable<i32>})}
    %1 = relalg.map %0 computes : [@map0::@tmp({type = !db.nullable<f64>})] (%arg0: !tuples.tuple){
      %5 = tuples.getcol %arg0 @items::@embedding : !db.nullable<!db.vector<3>>
      %6 = db.constant([3.000000e+00, 1.000000e+00, 2.000000e+00]) : !db.vector<3>
      %7 = db.l2_distance %5 : !db.nullable<!db.vector<3>>, %6 : !db.vector<3>
      tuples.return %7 : !db.nullable<f64>
    }
    %2 = relalg.sort %1 [(@map0::@tmp,asc)]
    %3 = relalg.limit 5 %2
    %4 = relalg.materialize %3 [@items::@id,@items::@embedding] => ["id", "embedding"] : <[id$0 : !db.nullable<i32>, embedding$0 : !db.nullable<!db.vector<3>>]>
    subop.set_result 0 %4 : !subop.result_table<[id$0 : !db.nullable<i32>, embedding$0 : !db.nullable<!db.vector<3>>]>
    return
  }
}
