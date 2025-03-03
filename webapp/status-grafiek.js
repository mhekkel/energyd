import * as d3 from 'd3';

let grafiek;

class Grafiek {
	constructor() {

		this.svg = d3.select("svg");
		this.svg.on("touchstart", event => event.preventDefault());

		const selector = document.getElementById("graph-date");

		selector.value = new Date().toISOString().substring(0, 10);

		selector.addEventListener("change", () => {
			console.log(selector.value);
			this.laadGrafiek();
		});

		this.maakGrafiek();
	}

	maakGrafiek() {
		const plotContainer = this.svg.node();
		let bBoxWidth = plotContainer.clientWidth;
		let bBoxHeight = plotContainer.clientHeight;

		if ((bBoxWidth * 9 / 16) > bBoxHeight)
			bBoxWidth = 16 * bBoxHeight / 9;
		else
			bBoxHeight = 9 * bBoxWidth / 16;

		this.margin = { top: 30, right: 50, bottom: 30, left: 50 };

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
			.text("Tijd");

		this.g = this.svg.append("g")
			.attr("transform", `translate(${this.margin.left},${this.margin.top})`);

		this.gX = this.g.append("g")
			.attr("class", "axis axis--x")
			.attr("transform", `translate(0,${this.height})`);

		this.gY1 = this.g.append("g")
			.attr("class", "axis axis--y");

		this.gY2 = this.g.append("g")
			.attr("class", "axis axis--y")
			.attr("transform", `translate(${this.width}, 0)`);

		this.plot = this.g.append("g")
			.attr("class", "plot")
			.attr("width", this.width)
			.attr("height", this.height)
			.attr("clip-path", "url(#clip)");

		this.plotData = this.plot.append('g')
			.attr("width", this.width)
			.attr("height", this.height);
	}

	herstelGrafiek() {
		this.svg.selectAll("#clip").remove();
		this.svg.selectAll("text").remove();
		this.svg.selectAll("g").remove();

		this.maakGrafiek();
		this.laadGrafiek();
	}

	async laadGrafiek() {

		this.plotData.selectAll("*").remove();

		const datum = new Date(document.getElementById("graph-date").value);
		const start = d3.timeDay.floor(datum);
		const stop = d3.timeDay.ceil(datum);

		if (this.width < 100)
			return;
		
		const resolutie = 15;

		const data = await fetch(`ajax/grafiek/${datum.toISOString().substring(0, 10)}?resolutie=${resolutie}`)
			.then(async r => {
				if (r.ok)
					return r.json();

				const error = await r.json();
				throw error.message;
			});

		console.log(data);

		let minE = 0, maxE = 0;

		data.forEach(d => {
			d.tijd = new Date(d.tijd);
			minE = Math.min(minE, d.zon, -d.batterij, -d.verbruik, d.levering, d.laad_niveau);
			maxE = Math.max(maxE, d.zon, -d.batterij, -d.verbruik, d.levering, d.laad_niveau);
		});

		const x = d3.scaleTime([start, stop], [1, this.width - 2]);
		const y_soc = d3.scaleLinear([0, 1], [this.height - 2, 1]);
		const y = d3.scaleLinear([minE, maxE], [this.height - 2, 1]);
		const colors = d3.scaleOrdinal(d3.schemeTableau10);

		const xFormat = d3.timeFormat("%H");
		const y_socFormat = d3.format(".0%");

		const xAxis = d3.axisBottom(x).tickFormat(xFormat).tickSizeOuter(0);//.ticks(d3.timeHour.every(1), xFormat);
		const y_Axis = d3.axisLeft(y);
		const y_soc_Axis = d3.axisRight(y_soc).tickFormat(y_socFormat)/* .tickSizeInner(this.width) */;

		this.gX.call(xAxis);
		this.gY1.call(y_Axis);
		this.gY2.call(y_soc_Axis);

		const barWidth = (this.width - 2) / ((24*60) / resolutie);

		const barData = [
			[0, "#9e8081", 'verbruik', -1],
			[0, "#7e867d", 'levering', 1],
			[1, d3.schemeTableau10[1], 'zon', 1],
			[2, d3.schemeTableau10[3], 'batterij', -1]
		];

		barData.forEach(([dx, color, bar, f]) => {
			this.plotData.append("g")
				.attr("fill", color)
				.selectAll(bar)
				.data(data)
				.join("rect")
				.attr("class", bar)
				.attr("x", d => x(d.tijd) + dx * (barWidth / 3))
				.attr("y", d => f * d[bar] > 0 ? y(f * d[bar]) : y(0))
				.attr("width", Math.max(barWidth / 3 - 1, 1))
				.attr("height", d => Math.abs(y(0) - y(f * d[bar])));
		});

		const line_soc = d3.line()
			.curve(d3.curveBasis)
			.x(d => x(d.tijd))
			.y(d => y_soc(d.laad_niveau));

		this.plotData.append("path")
			.attr("class", "soc")
			.attr("fill", "none")
			.attr("stroke", colors(0))
			.attr("stroke-width", 1.5)
			.attr("d", line_soc(data));

		const line_netto = d3.line()
			.curve(d3.curveBasis)
			.x(d => x(d.tijd))
			.y(d => y(d.levering - d.verbruik));

		this.plotData.append("path")
			.attr("class", "netto")
			.attr("fill", "none")
			.attr("stroke", "black")
			.attr("stroke-width", 1.5)
			.attr("d", line_netto(data));
	}
}

window.addEventListener("load", () => {
	grafiek = new Grafiek();
	grafiek.laadGrafiek();
});

window.addEventListener("resize", () => {
	if (grafiek) {
		grafiek.herstelGrafiek();
	}
})