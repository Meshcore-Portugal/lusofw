const crypto = require('crypto');

// We use SHA-256 and Little Endian (matching the ESP32/nRF52 implementation) for the hash calculation
function calculateNodeHash(nodeName, byte0, byte1, byte2, byte3) {
	const nameBuffer = Buffer.from(nodeName, 'utf8');
	const idBuffer = Buffer.from([byte0, byte1, byte2, byte3]);
	const combinedBuffer = Buffer.concat([nameBuffer, idBuffer]);

	const hashResult = crypto.createHash('sha256').update(combinedBuffer).digest();

	// Read the first 4 bytes of the hash in Little Endian format (UInt32)
	return hashResult.readUInt32LE(0);
}

function parseNodeIdBytes(nodeId) {
	if (nodeId.length < 8) {
		throw new Error("Node ID must have at least 8 hex characters for the 4 bytes (e.g., 877774ff).");
	}
	return {
		byte0: parseInt(nodeId.substring(0, 2), 16),
		byte1: parseInt(nodeId.substring(2, 4), 16),
		byte2: parseInt(nodeId.substring(4, 6), 16),
		byte3: parseInt(nodeId.substring(6, 8), 16)
	};
}

// The window in LusoFW for smart-adverts in the final simulations is 23 hours!
const WINDOW_SECONDS = 23 * 3600;

// Script entry logic
const args = process.argv.slice(2);
if (args.length < 2) {
	console.log("Usage: node next_adverts.js <node_name> <node_id_4_bytes>");
	console.log("Example: node next_adverts.js \"Testing 123\" 877774ff");
    console.log("Or without quotes: node next_adverts.js Testing 123 877774ff");
	process.exit(1);
}

// Join all arguments except the last one to form the name (allows using spaces without quotes)
const nodeName = args.slice(0, args.length - 1).join(' ');
const nodeId = args[args.length - 1];

const { byte0, byte1, byte2, byte3 } = parseNodeIdBytes(nodeId);
const hash = calculateNodeHash(nodeName, byte0, byte1, byte2, byte3);
const offset = hash % WINDOW_SECONDS;

const nowEpoch = Math.floor(Date.now() / 1000);
let current_cycle_start = nowEpoch - (nowEpoch % WINDOW_SECONDS);

console.log(`\nAnalyzing adverts for node: "${nodeName}" (ID: ${nodeId})`);
console.log(`Fixed offset within the 23h window: ${offset} seconds\n`);

console.log("Expected adverts (-5 to +5 days):");
console.log("--------------------------------------------------");

for (let i = -5; i <= 5; i++) {
	let cycle_start = current_cycle_start + (i * WINDOW_SECONDS);
	let target = cycle_start + offset;

	const jitter = ((hash ^ cycle_start) % 7) - 3;
	const finalEpoch = target + jitter;

	const dTarget = new Date(finalEpoch * 1000).toISOString().replace('T', ' ').substring(0, 19);
	const status = finalEpoch < nowEpoch ? "[PAST]" : "[UPCOMING]";
	const dayLabel = i === 0 ? "Current" : (i > 0 ? `+${i} Day${i > 1 ? 's' : ''}` : `${i} Day${i < -1 ? 's' : ''}`);

	console.log(`Advert ${dayLabel.padEnd(8)}: ${dTarget} UTC (Jitter: ${jitter > 0 ? '+' : ''}${jitter}s) ${status}`);
}
console.log("--------------------------------------------------\n");
