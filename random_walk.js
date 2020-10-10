let v = [1.0, 0.0, 0.0, 0.0];
for (let i = 0; i < 1000000; i++) {
	v[v.length - 1] = Math.random() - 0.5;
	for (let j = 0; j < v.length - 1; j++) {
		v[j] = v[j] * 0.99 + v[j + 1] * 0.1;
	}
	process.stdout.write(JSON.stringify({ x: v[0] }) + '\n');
}
