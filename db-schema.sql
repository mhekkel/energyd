-- drop table if exists public.p1_opname;
drop table if exists public.sessy_soc;

-- P1 status

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

-- Sessy state of charge

create table
	public.sessy_soc (
		tijd timestamp without time zone default now() not null,
		nr integer not null,
		soc numeric(4, 3)
	);

alter table only public.sessy_soc
add constraint sessy_soc_pk primary key (tijd, nr);

alter table public.sessy_soc owner to "energie-admin";

-- Status graph data

create table
	public.daily_graph (
		tijd timestamp without time zone default now() not null,
		soc numeric(3, 2),
		batterij numeric(4),
		verbruik numeric(4),
		levering numeric(4),
		opwekking numeric(4)
	);

alter table public.daily_graph owner to "energie-admin";

