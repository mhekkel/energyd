import '@babel/polyfill'
import '@fortawesome/fontawesome-free/css/all.min.css';
import 'bootstrap';
import 'bootstrap/js/dist/modal'
import 'bootstrap/dist/css/bootstrap.min.css';

class OpnameEditor {
	constructor() {
		this.form = document.getElementById("opname-form");
		// this.csrf = this.form.elements['_csrf'].value;

		this.form.addEventListener("submit", (evt) => this.saveOpname(evt));
		this.id = this.form["id"].value;
	}

	saveOpname(e) {
		if (e)
			e.preventDefault();

		this.opname = { standen: {} };
		for (let key in this.form.elements) {
			const input = this.form.elements[key];
			if (input.type !== 'number') continue;
			this.opname.standen[`${input.dataset.id}`] = +input.value;
		}

		const url = this.id ? `ajax/opname/${this.id}` : 'ajax/opname';
		const method = this.id ? 'put' : 'post';

		fetch(url, {
			credentials: "include",
			headers: {
				'Accept': 'application/json',
				'Content-Type': 'application/json',
				// 'X-CSRF-Token': this.csrf
			},
			method: method,
			body: JSON.stringify(this.opname)
		}).then(async response => {
			if (response.ok)
				return response.json();

			const error = await response.json();
			console.log(error);
			throw error.message;
		}).then(r => {
			console.log(r);
			$(this.dialog).modal('hide');

			window.location = "opnames";
		}).catch(err => alert(err));
	}
}

window.addEventListener("load", () => {
	new OpnameEditor();
});
