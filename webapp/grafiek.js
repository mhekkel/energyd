import '@babel/polyfill'
import '@fortawesome/fontawesome-free/css/all.min.css';
import 'bootstrap';
import 'bootstrap/js/dist/modal'
import 'bootstrap/dist/css/bootstrap.min.css';
import * as d3 from 'd3';

class grafiek {
	constructor() {

		this.selector = document.getElementById('grafiek-id');
		this.selector.addEventListener('change', ev => {
			if (ev)
				ev.preventDefault();
			this.laadGrafiek();
		});

		for (let btn of document.getElementsByClassName("aggr-btn")) {
			if (btn.checked)
				this.aggrType = btn.dataset.aggr;
			btn.onchange = (e) => this.selectAggrType(e.target.dataset.aggr);
		}

		this.svg = d3.select("svg");

		const plotContainer = $(this.svg.node());
		let bBoxWidth = plotContainer.width();
		let bBoxHeight = plotContainer.height();

		if ((bBoxWidth * 9 / 16) > bBoxHeight)
			bBoxWidth = 16 * bBoxHeight / 9;
		else
			bBoxHeight = 9 * bBoxWidth / 16;

		this.margin = {top: 30, right: 50, bottom: 30, left: 50};

		this.width = bBoxWidth - this.margin.left - this.margin.right;
		this.height = bBoxHeight - this.margin.top - this.margin.bottom;

		this.defs = this.svg.append('defs');

		this.defs
			.append("svg:clipPath")
			.attr("id", "clip")
			.append("svg:rect")
			.attr("x", 0)
			.attr("y", 0)
			.attr("width", this.width)
			.attr("height", this.height);

		this.svg.append("text")
			.attr("class", "x axis-label")
			.attr("text-anchor", "end")
			.attr("x", this.width + this.margin.left)
			.attr("y", this.height + this.margin.top + this.margin.bottom)
			.text("Datum");

		this.svg.append("text")
			.attr("class", "y axis-label")
			.attr("text-anchor", "end")
			.attr("x", -this.margin.top)
			.attr("y", 6)
			.attr("dy", ".75em")
			.attr("transform", "rotate(-90)")
			.text("Verbruik per dag");

		this.g = this.svg.append("g")
			.attr("transform", `translate(${this.margin.left},${this.margin.top})`);

		this.gX = this.g.append("g")
			.attr("class", "axis axis--x")
			.attr("transform", `translate(0,${this.height})`);

		this.gY = this.g.append("g")
			.attr("class", "axis axis--y");

		this.plot = this.g.append("g")
			.attr("class", "plot")
			.attr("width", this.width)
			.attr("height", this.height)
			.attr("clip-path", "url(#clip)");

		this.plotData = this.plot.append('g')
			.attr("width", this.width)
			.attr("height", this.height);

		const zoom = d3.zoom()
			.scaleExtent([1, 40])
			.translateExtent([[0, 0], [this.width + 90, this.height + 90]])
			.on("zoom", () => this.zoomed());

		this.svg.call(zoom);
	}
	
	laadGrafiek() {
		const selected = this.selector.selectedOptions;
		if (selected.length !== 1)
			throw("ongeldige keuze");

		const keuze = selected.item(0).value;
		const grafiekNaam = selected.item(0).textContent;

		let grafiekTitel = $(".grafiek-titel");
		if (grafiekTitel.hasClass("grafiek-status-loading"))  // avoid multiple runs
			return;

		grafiekTitel
			.addClass("grafiek-status-loading")
			.removeClass("grafiek-status-loaded")
			.removeClass("grafiek-status-failed");

		Array.from(document.getElementsByClassName("grafiek-naam"))
			.forEach(span => span.textContent = grafiekNaam);

		fetch(`ajax/data/${keuze}/${this.aggrType}`, {
			credentials: "include",
			headers: {
				'Accept': 'application/json'
			}
		}).then(async response => {
			if (response.ok)
				return response.json();

			const error = await response.json();
			console.log(error);
			throw error.message;
		}).then(data => {
			this.processData(data);
			grafiekTitel.removeClass("grafiek-status-loading").addClass("grafiek-status-loaded");
		}).catch(err => {
			grafiekTitel.removeClass("grafiek-status-loading").addClass("grafiek-status-failed");
			console.log(err);
		});
	}

	processData(data) {
		const dataPunten = [];
		const years = new Set();
		let lastYear = 0;

		for (let d in data.punten)
		{
			const date = new Date(d);

			const year = date.getFullYear();
			years.add(year);
			if (lastYear < year)
				lastYear = year;

			dataPunten.push({
				date: date,
				year: date.getFullYear(),
				xdate: d3.timeFormat("%j")(date),
				verbruik: data.punten[d]
			});
		}

		const dataPunten2 = d3.nest()
			.key(d => d.date.getFullYear())
			.entries(dataPunten);
		
		const colors = d3.scaleOrdinal([...years].reverse(), d3.schemeCategory10);

		const domX = [1, 366];

		const tickFormat = t => d3.timeFormat("%b")(d3.timeParse("%j")(t));

		const x = d3.scaleLinear().domain(domX).range([1, this.width - 2]);

		const xTicks = d3.timeMonth
			.every(1)
			.range(new Date(2000, 0, 1), new Date(2000, 11, 31))
			.map(d3.timeFormat("%j"));

		const xAxis = d3.axisBottom(x).tickValues(xTicks).tickFormat(tickFormat);

		this.gX.call(xAxis);

		const domY = d3.extent(dataPunten, d => d.verbruik);
		const y = d3.scaleLinear().domain(domY).range([this.height - 2, 1]);
		const yAxis = d3.axisLeft(y).tickSizeInner(-this.width);

		this.gY.call(yAxis);

		this.plotData.selectAll(".line")
			.remove();
		
		this.plotData.selectAll(".line")
			.data(dataPunten2)
			.enter()
			.append("path")
				.attr("class", "line")
				.attr("fill", "none")
				.attr("opacity", d => 1 / (lastYear - d.key + 1))
				// .attr("opacity", d => 1.5 / (lastYear - d.key + 1.5))
				.attr("stroke", d => colors(d.key))
				.attr("stroke-width", 1.5)
				.attr("d", d => 
					d3.line()
						.x(d => x(d.xdate))
						.y(d => y(d.verbruik))(d.values)
				);

		// voortschrijdend gemiddelde

		const dataPunten3 = [];
		for (let d in data.vsgem)
		{
			const date = new Date(d);

			if (! years.has(date.getFullYear()))
				console.log("how is this possible, year not known yet");

			dataPunten3.push({
				date: date,
				year: date.getFullYear(),
				xdate: d3.timeFormat("%j")(date),
				verbruik: data.vsgem[d]
			});
		}

		const dataPunten4 = d3.nest()
			.key(d => d.date.getFullYear())
			.entries(dataPunten3);
		
		this.plotData.selectAll(".ma-line")
			.remove();
		
		this.plotData.selectAll(".ma-line")
			.data(dataPunten4)
			.enter()
			.append("path")
				.attr("class", "ma-line")
				.attr("fill", "none")
				.attr("stroke", "gray")
				.attr("stroke-width", 2)
				.attr("d", d => 
					d3.line()
						.x(d => x(d.xdate))
						.y(d => y(d.verbruik))(d.values)
				);

	}

	zoomed() {

	}

	selectAggrType(aggr) {
		this.aggrType = aggr;
		this.laadGrafiek()
	}
}


window.addEventListener("load", () => {
	const g = new grafiek();
	g.laadGrafiek();
});
