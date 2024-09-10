-- The table for the daily graph (status panel)

drop table if exists public.daily_graph;

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

-- Tables for the history of energy usage

drop table if exists public.opname cascade;

create table public.opname (
    id serial primary key,
    tijd timestamp without time zone default now() not null
);

alter table public.opname owner to "energie-admin";
comment on table public.opname is 'een opname van meerdere tellerstanden';

create table public.teller (
    id serial primary key,
    naam character varying(32) not null,
    naam_kort character varying(16) not null,
    schaal integer not null,
    teken integer
);

alter table public.teller owner to "energie-admin";
comment on table public.teller is 'de verschillende tellers';

create table public.tellerstand (
    id serial primary key,
    teller_id integer references public.teller,
    stand numeric(8,3) not null,
    opname_id integer references public.opname
);

alter table public.tellerstand owner to "energie-admin";
comment on table public.tellerstand is 'de stand van een teller';

-- some data

copy public.teller (id, naam, naam_kort, schaal, teken) from stdin;
1	Warmte	Warmte	3	1
2	Electriciteit verbruik laag	Verbruik laag	0	1
3	Electriciteit verbruik hoog	Verbruik hoog	0	1
5	Electriciteit teruglevering hoog	Teruglevering h	0	-1
4	Electriciteit teruglevering laag	Teruglevering l	0	-1
\.
