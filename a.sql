select col1, col2, (select count(*) from table2) from dba_tables where table_name like "%XE" and b=1 or c is not null order by 1;

create table t (
id integer,
name char(32),
score float,
);
