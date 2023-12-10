drop table if exists public.p1_opname;

create table
	public.p1_opname (
		tijd timestamp without time zone default now() not null,
		verbruik_hoog numeric(9, 3),
		verbruik_laag numeric(9, 3),
		levering_hoog numeric(9, 3),
		levering_laag numeric(9, 3)
	);

alter table only public.p1_opname
add constraint p1_opname_pk primary key (tijd);

alter table public.p1_opname owner to "energie-admin";