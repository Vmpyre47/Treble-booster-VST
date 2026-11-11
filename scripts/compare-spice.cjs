const fs=require('fs'); const folder=process.argv[2]||'test-results/spice';
function read(path){return fs.readFileSync(path,'utf8').trim().split(/\r?\n/).map(s=>s.trim().split(/\s+/).map(Number)).filter(r=>r.length>=2&&Number.isFinite(r[0])&&Number.isFinite(r[1]));}
for(const mode of ['ge','si'])for(const boost of ['1','0.5']) {
 const prefix=`${folder}/${mode}-${boost}`;
 const a=read(prefix+'-spice.txt'),b=read(prefix+'-cpp.txt');
 let sum=0,ref=0,n=0,max=0;
 for(let i=48000;i<Math.min(a.length,b.length);i++) {
  const d=b[i][1]-a[i][1];sum+=d*d;ref+=a[i][1]*a[i][1];max=Math.max(max,Math.abs(d));n++;
 }
 console.log(`${mode} Boost=${boost} reference_RMS=${Math.sqrt(ref/n)} relative_RMS_error=${Math.sqrt(sum/ref)*100}% max_abs_error=${max}V samples=${n}`);
}
