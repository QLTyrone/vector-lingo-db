module @querymodule{
    func @main (%executionContext: !util.generic_memref<i8>)  -> !db.table{
        %1 = relalg.basetable @part { table_identifier="part", rows=20000 , pkey=["p_partkey"]} columns: {p_partkey => @p_partkey({type=!db.int<64>}),
            p_name => @p_name({type=!db.string}),
            p_mfgr => @p_mfgr({type=!db.string}),
            p_brand => @p_brand({type=!db.string}),
            p_type => @p_type({type=!db.string}),
            p_size => @p_size({type=!db.int<32>}),
            p_container => @p_container({type=!db.string}),
            p_retailprice => @p_retailprice({type=!db.decimal<15,2>}),
            p_comment => @p_comment({type=!db.string})
        }
        %2 = relalg.basetable @supplier { table_identifier="supplier", rows=1000 , pkey=["s_suppkey"]} columns: {s_suppkey => @s_suppkey({type=!db.int<64>}),
            s_name => @s_name({type=!db.string}),
            s_address => @s_address({type=!db.string}),
            s_nationkey => @s_nationkey({type=!db.int<64>}),
            s_phone => @s_phone({type=!db.string}),
            s_acctbal => @s_acctbal({type=!db.decimal<15,2>}),
            s_comment => @s_comment({type=!db.string})
        }
        %3 = relalg.crossproduct %1, %2
        %4 = relalg.basetable @partsupp { table_identifier="partsupp", rows=80000 , pkey=["ps_partkey","ps_suppkey"]} columns: {ps_partkey => @ps_partkey({type=!db.int<64>}),
            ps_suppkey => @ps_suppkey({type=!db.int<64>}),
            ps_availqty => @ps_availqty({type=!db.int<32>}),
            ps_supplycost => @ps_supplycost({type=!db.decimal<15,2>}),
            ps_comment => @ps_comment({type=!db.string})
        }
        %5 = relalg.crossproduct %3, %4
        %6 = relalg.basetable @nation { table_identifier="nation", rows=25 , pkey=["n_nationkey"]} columns: {n_nationkey => @n_nationkey({type=!db.int<64>}),
            n_name => @n_name({type=!db.string}),
            n_regionkey => @n_regionkey({type=!db.int<64>}),
            n_comment => @n_comment({type=!db.string<nullable>})
        }
        %7 = relalg.crossproduct %5, %6
        %8 = relalg.basetable @region { table_identifier="region", rows=5 , pkey=["r_regionkey"]} columns: {r_regionkey => @r_regionkey({type=!db.int<64>}),
            r_name => @r_name({type=!db.string}),
            r_comment => @r_comment({type=!db.string<nullable>})
        }
        %9 = relalg.crossproduct %7, %8
        %11 = relalg.selection %9(%10: !relalg.tuple) {
            %12 = relalg.getattr %10 @part::@p_partkey : !db.int<64>
            %13 = relalg.getattr %10 @partsupp::@ps_partkey : !db.int<64>
            %14 = db.compare eq %12 : !db.int<64>,%13 : !db.int<64>
            %15 = relalg.getattr %10 @supplier::@s_suppkey : !db.int<64>
            %16 = relalg.getattr %10 @partsupp::@ps_suppkey : !db.int<64>
            %17 = db.compare eq %15 : !db.int<64>,%16 : !db.int<64>
            %18 = relalg.getattr %10 @part::@p_size : !db.int<32>
            %19 = db.constant (15) :!db.int<64>
            %20 = db.cast %18 : !db.int<32> -> !db.int<64>
            %21 = db.compare eq %20 : !db.int<64>,%19 : !db.int<64>
            %22 = relalg.getattr %10 @part::@p_type : !db.string
            %23 = db.constant ("%BRASS") :!db.string
            %24 = db.compare like %22 : !db.string,%23 : !db.string
            %25 = relalg.getattr %10 @supplier::@s_nationkey : !db.int<64>
            %26 = relalg.getattr %10 @nation::@n_nationkey : !db.int<64>
            %27 = db.compare eq %25 : !db.int<64>,%26 : !db.int<64>
            %28 = relalg.getattr %10 @nation::@n_regionkey : !db.int<64>
            %29 = relalg.getattr %10 @region::@r_regionkey : !db.int<64>
            %30 = db.compare eq %28 : !db.int<64>,%29 : !db.int<64>
            %31 = relalg.getattr %10 @region::@r_name : !db.string
            %32 = db.constant ("EUROPE") :!db.string
            %33 = db.compare eq %31 : !db.string,%32 : !db.string
            %34 = relalg.getattr %10 @partsupp::@ps_supplycost : !db.decimal<15,2>
            %35 = relalg.basetable @partsupp1 { table_identifier="partsupp", rows=80000 , pkey=["ps_partkey","ps_suppkey"]} columns: {ps_partkey => @ps_partkey({type=!db.int<64>}),
                ps_suppkey => @ps_suppkey({type=!db.int<64>}),
                ps_availqty => @ps_availqty({type=!db.int<32>}),
                ps_supplycost => @ps_supplycost({type=!db.decimal<15,2>}),
                ps_comment => @ps_comment({type=!db.string})
            }
            %36 = relalg.basetable @supplier1 { table_identifier="supplier", rows=1000 , pkey=["s_suppkey"]} columns: {s_suppkey => @s_suppkey({type=!db.int<64>}),
                s_name => @s_name({type=!db.string}),
                s_address => @s_address({type=!db.string}),
                s_nationkey => @s_nationkey({type=!db.int<64>}),
                s_phone => @s_phone({type=!db.string}),
                s_acctbal => @s_acctbal({type=!db.decimal<15,2>}),
                s_comment => @s_comment({type=!db.string})
            }
            %37 = relalg.crossproduct %35, %36
            %38 = relalg.basetable @nation1 { table_identifier="nation", rows=25 , pkey=["n_nationkey"]} columns: {n_nationkey => @n_nationkey({type=!db.int<64>}),
                n_name => @n_name({type=!db.string}),
                n_regionkey => @n_regionkey({type=!db.int<64>}),
                n_comment => @n_comment({type=!db.string<nullable>})
            }
            %39 = relalg.crossproduct %37, %38
            %40 = relalg.basetable @region1 { table_identifier="region", rows=5 , pkey=["r_regionkey"]} columns: {r_regionkey => @r_regionkey({type=!db.int<64>}),
                r_name => @r_name({type=!db.string}),
                r_comment => @r_comment({type=!db.string<nullable>})
            }
            %41 = relalg.crossproduct %39, %40
            %43 = relalg.selection %41(%42: !relalg.tuple) {
                %44 = relalg.getattr %10 @part::@p_partkey : !db.int<64>
                %45 = relalg.getattr %42 @partsupp1::@ps_partkey : !db.int<64>
                %46 = db.compare eq %44 : !db.int<64>,%45 : !db.int<64>
                %47 = relalg.getattr %42 @supplier1::@s_suppkey : !db.int<64>
                %48 = relalg.getattr %42 @partsupp1::@ps_suppkey : !db.int<64>
                %49 = db.compare eq %47 : !db.int<64>,%48 : !db.int<64>
                %50 = relalg.getattr %42 @supplier1::@s_nationkey : !db.int<64>
                %51 = relalg.getattr %42 @nation1::@n_nationkey : !db.int<64>
                %52 = db.compare eq %50 : !db.int<64>,%51 : !db.int<64>
                %53 = relalg.getattr %42 @nation1::@n_regionkey : !db.int<64>
                %54 = relalg.getattr %42 @region1::@r_regionkey : !db.int<64>
                %55 = db.compare eq %53 : !db.int<64>,%54 : !db.int<64>
                %56 = relalg.getattr %42 @region1::@r_name : !db.string
                %57 = db.constant ("EUROPE") :!db.string
                %58 = db.compare eq %56 : !db.string,%57 : !db.string
                %59 = db.and %46 : !db.bool,%49 : !db.bool,%52 : !db.bool,%55 : !db.bool,%58 : !db.bool
                relalg.return %59 : !db.bool
            }
            %61 = relalg.aggregation @aggr1 %43 [] (%60 : !relalg.relation) {
                %62 = relalg.aggrfn min @partsupp1::@ps_supplycost %60 : !db.decimal<15,2,nullable>
                relalg.addattr @aggfmname1({type=!db.decimal<15,2,nullable>}) %62
                relalg.return
            }
            %63 = relalg.getscalar @aggr1::@aggfmname1 %61 : !db.decimal<15,2,nullable>
            %64 = db.compare eq %34 : !db.decimal<15,2>,%63 : !db.decimal<15,2,nullable>
            %65 = db.and %14 : !db.bool,%17 : !db.bool,%21 : !db.bool,%24 : !db.bool,%27 : !db.bool,%30 : !db.bool,%33 : !db.bool,%64 : !db.bool<nullable>
            relalg.return %65 : !db.bool<nullable>
        }
        %66 = relalg.sort %11 [(@supplier::@s_acctbal,desc),(@nation::@n_name,asc),(@supplier::@s_name,asc),(@part::@p_partkey,asc)]
        %67 = relalg.limit 100 %66
        %68 = relalg.materialize %67 [@supplier::@s_acctbal,@supplier::@s_name,@nation::@n_name,@part::@p_partkey,@part::@p_mfgr,@supplier::@s_address,@supplier::@s_phone,@supplier::@s_comment] => ["s_acctbal","s_name","n_name","p_partkey","p_mfgr","s_address","s_phone","s_comment"] : !db.table
        return %68 : !db.table
    }
}
